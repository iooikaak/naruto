//
// Created by 王振奎 on 2020/9/18.
//
#include <chrono>
#include <gflags/gflags.h>

#include "protocol/message_type.h"
#include "protocol/replication.pb.h"
#include "replication.h"

DEFINE_int32(repl_back_log_size, 7290, "listen port");
static bool valid_repl_back_log_size(const char* flagname, int value){
    return true;
}
DEFINE_validator(repl_back_log_size,&valid_repl_back_log_size);

DEFINE_int32(repl_timeout_sec, 1, "listen port");
static bool valid_repl_timeout_sec(const char* flagname, int value){
    return true;
}
DEFINE_validator(repl_timeout_sec,&valid_repl_timeout_sec);

DEFINE_double(repl_cron_interval, 1, "listen port");
static bool valid_repl_cron_interval(const char* flagname, double value){
    return true;
}
DEFINE_validator(repl_cron_interval,&valid_repl_cron_interval);


naruto::Replication::Replication(int merge_threads_size) {
    _master_host = "";
    _master_port = 0;
    _is_master = false;
    _repl_timeout = FLAGS_repl_timeout_sec;
    _repl_incr_id.store(0);
    _pos.store(0);
    _repl.reserve(merge_threads_size);
    _back_log.reserve(0);
    _master_repl_offset = 0;
    _repl_ping_slave_period = 0;
    _repl_back_size  = FLAGS_repl_back_log_size;
    _repl_backlog_histlen = 0;
    _repl_backlog_idx = 0;
    _repl_backlog_off = 0;
    _repl_state = state::NONE;
    _repl_transfer_size = 0;
    _repl_transfer_read = 0;
    _repl_transfer_last_fsync_off = 0;
    _repl_transfer_s = 0;
    _repl_transfer_fd = 0;
    _repl_transfer_tmpfile = "";
    _repl_transfer_lastio = std::chrono::steady_clock::now();
    _repl_slave_ro = false;
    _repl_down_since = std::chrono::steady_clock::now();
    _repl_master_runid = "";
    _repl_master_initial_offset = 0;

    cron_interval_ = FLAGS_repl_cron_interval;
    time_watcher_.set<Replication, &Replication::onReplCron>(this);
    time_watcher_.set(ev::get_default_loop());
    time_watcher_.start(cron_interval_, cron_interval_);
}

void naruto::Replication::onReplCron(ev::timer &, int) {
    LOG(INFO) << "onReplCron...";
    // ====================== slave 情况 ===========================
    auto now = std::chrono::steady_clock::now();
    // 连接 master 超时
    if (!_master_host.empty() && (_repl_state == state::CONNECTING||
        _repl_state == state::RECEIVE_PONG) && (now - _repl_transfer_lastio).count() > _repl_timeout){
        LOG(WARNING) <<  "Timeout connecting to the MASTER...";
        _undo_connect_master();
    }

    // dump db 传送超时
    if (!_master_host.empty() && _repl_state == state::TRANSFOR
            && (now - _repl_transfer_lastio).count() > _repl_timeout){
        LOG(WARNING) << "Timeout receiving bulk data from MASTER... If the problem persists try to set the 'repl-timeout' parameter in naruto.conf to a larger value.";
        _abort_sync_transfer();
    }

    // 从服务器曾经连接上主服务器，但现在超时
    if (!_master_host.empty() && _repl_state ==state::CONNECTED &&
        (now - _repl_last_interaction).count() > _repl_timeout){
        LOG(WARNING) <<"MASTER timeout: no data nor PING received...";
        _free_client(_master);
    }

    // 尝试连接主服务器
    if (!_master_host.empty() && _repl_state ==state::CONNECT){ _connect_master(); }

    // 定期向主服务器发送 ACK 命令
    if (!_master_host.empty() && _master){ _repl_send_ack(); }

    // ====================== master 情况 ===========================
    // 如果服务器有从服务器，定时向它们发送 PING 。
    // 这样从服务器就可以实现显式的 master 超时判断机制，
    // 即使 TCP 连接未断开也是如此。
    if ((_cronloops % _repl_ping_slave_period) == 0 && !_slaves.empty()){
        replication::command_ping ping;
        ping.set_ip("127.0.0.1");
        feedSlaves(ping, COMMAND_REPL_PING);
    }

    // 断开超时从服务器
    for (auto it = _slaves.begin(); it != _slaves.end(); ++it){
        if ((*it)->repl_state != state::ONLINE) continue;
        if ((_repl_unixtime - (*it)->repl_ack_time).count() > _repl_timeout){
            auto slave =  *it;
            _slaves.erase(it);
            _free_client(slave);
        }
    }
}

void naruto::Replication::onEvents(ev::io &, int) {

}

void naruto::Replication::onSyncWithMaster(ev::io & watcher, int event) {
    if (_repl_state ==state::NONE) {
        ::close(watcher.fd);
        return;
    }
    int sockerr = 0;
    int type = 0;

    socklen_t errlen = sizeof(sockerr);
    // 检查套接字错误
    if (getsockopt(watcher.fd, SOL_SOCKET, SO_ERROR, &sockerr, &errlen) == -1)
        sockerr = errno;
    if (sockerr) {
        _repl_ev_io_w->stop();
        _repl_ev_io_r->stop();
        LOG(WARNING) << "Error condition on socket for SYNC: " << strerror(sockerr);
        goto error;
    }

    // 如果状态为 CONNECTING ，那么在进行初次同步之前，
    // 向主服务器发送一个非阻塞的 PING
    // 因为接下来的 RDB 文件发送非常耗时，所以我们想确认主服务器真的能访问
    if (_repl_state == state::CONNECTING){
        LOG(INFO) << "Repl state CONNECTING, send PING to master";
        _repl_state = state::RECEIVE_PONG;
        _repl_ev_io_r->stop();
        replication::command_ping ping;
        ping.set_ip("1111");
        _master->sendMsg(ping, COMMAND_REPL_PING);
        // 返回，等待 PONG 到达
        return;
    }

    if (_repl_state == state::RECEIVE_PONG){
        LOG(INFO) << "Repl state RECEIVE_PONG";
        _repl_ev_io_w->stop();
        replication::command_pong pong;
        uint16_t type = _master->read(pong);
        if (type != COMMAND_REPL_PONG){
            LOG(WARNING) << "Repl receive not pong";
            goto error;
        }else{
            // 接收到 PONG
            LOG(INFO) << "Master replied to PING, replication can continue...";
        }
    }

    type = _slave_try_partial_resynchronization();
    if (type == COMMAND_REPL_PARTSYNC) {
        LOG(INFO) << "MASTER <-> SLAVE sync: Master accepted a Partial Resynchronization.";
        return;
    }else if (type == COMMAND_REPL_FULLSYNC){

    }

    // 走到这则type不可识别
error:
    ::close(watcher.fd);
    _repl_transfer_fd = -1;
    _repl_state = state::CONNECT;
    return;
}

void naruto::Replication::feedSlaves(const ::google::protobuf::Message & msg, uint16_t type) {
    utils::Bytes pack(32);
    pack.putMessage(msg, type);
    // 写到复制积压缓冲区
    _feed_backlog(pack);

    for (const auto& slave : _slaves){
        // 不要给正在等待 BGSAVE 开始的从服务器发送命令
        if (slave->repl_state == state::WAIT_BGSAVE_START) continue;
        // 向已经接收完 RDB 文件的从服务器发送命令
        try {
            slave->connect->send(pack);
        }catch (std::exception& e){
            LOG(ERROR) << "Feed slave:" << e.what();
        }
    }
}

void naruto::Replication::_feed_backlog(utils::Bytes& bytes) {
    size_t len = bytes.size();
    _master_repl_offset += (long long)len;

    while (len){
        // 从 idx 到 backlog 尾部的字节数
        size_t thislen = _repl_back_size - _repl_backlog_idx;
        // 如果 idx 到 backlog 尾部这段空间足以容纳要写入的内容
        // 那么直接将写入数据长度设为 len
        // 在将这些 len 字节复制之后，这个 while 循环将跳出
        if (thislen > len) thislen = len;
        for (int i = 0; i < thislen; ++i) {
            _back_log.push_back(bytes.get(i));
        }
        // 更新 idx ，指向新写入的数据之后
        _repl_backlog_idx += thislen;
        // 如果写入达到尾部，那么将索引重置到头部
        if (_repl_backlog_idx == _repl_back_size)
            _repl_backlog_idx = 0;
        len -= thislen;
        _repl_backlog_histlen += thislen;
    }

    // histlen 的最大值只能等于 backlog_size
    // 另外，当 histlen 大于 repl_backlog_size 时，
    // 表示写入数据的前头有一部分数据被自己的尾部覆盖了
    if (_repl_backlog_histlen > _repl_back_size)
        _repl_backlog_histlen = _repl_back_size;

    // 记录程序可以依靠 backlog 来还原的数据的第一个字节的偏移量
    // 比如 master_repl_offset = 10086
    // repl_backlog_histlen = 30
    // 那么 backlog 所保存的数据的第一个字节的偏移量为
    // 10086 - 30 + 1 = 10056 + 1 = 10057
    // 这说明如果从服务器如果从 10057 至 10086 之间的任何时间断线
    // 那么从服务器都可以使用 PSYNC
    _repl_backlog_off = _master_repl_offset - _repl_backlog_histlen + 1;
}


void naruto::Replication::_undo_connect_master() {
    int fd = _repl_transfer_s;
    assert(_repl_state == state::CONNECTING || _repl_state == state::RECEIVE_PONG);
    _repl_ev_io_r->stop();
    _repl_ev_io_w->stop();
    ::close(fd);
    _repl_transfer_s = -1;
    _repl_state = state::CONNECT;
}

void naruto::Replication::_connect_master() {
    connection::ConnectOptions ops;
    ops.host = _master_host;
    ops.port = _master_port;

    LOG(ERROR) << "Try connecting to MASTER "<< ops.host << ":" << ops.port;
    // 连接并监听主服务的读写事件
    _master = std::make_shared<narutoClient>();
    _master->connect = std::make_shared<naruto::connection::Connect>(ops);

    if (_master->connect->connect() != CONNECT_RT_OK){
        LOG(ERROR) << "Fail connecting to MASTER "<< ops.host << ":" << ops.port << ",errmsg:"
            << _master->connect->errmsg();
        return;
    }
    _master->flags |= (uint32_t)CLIENT_FLAG_MASTER;
    LOG(INFO) << "MASTER <-> SLAVE sync started";
    _repl_ev_io_r = std::make_shared<ev::io>();
    _repl_ev_io_r->set<Replication, &Replication::onSyncWithMaster>(this);
    _repl_ev_io_r->set(ev::get_default_loop());
    _repl_ev_io_r->start(_master->connect->fd(), ev::READ);

    _repl_ev_io_w = std::make_shared<ev::io>();
    _repl_ev_io_w->set<Replication, &Replication::onSyncWithMaster>(this);
    _repl_ev_io_w->set(ev::get_default_loop());
    _repl_ev_io_w->start(_master->connect->fd(), ev::WRITE);

    // 初始化统计变量
    _repl_transfer_lastio = std::chrono::steady_clock::now();
    _repl_transfer_fd = _master->connect->fd();
    _repl_state = state::CONNECTING;
}

// 停止下载 RDB 文件
void naruto::Replication::_abort_sync_transfer() {
    assert(_repl_state == state::TRANSFOR);
    _repl_ev_io_r->stop();
    ::close(_repl_transfer_fd);
    ::unlink(_repl_transfer_tmpfile.c_str());
    _repl_transfer_tmpfile = "";
    _repl_state = state::CONNECT;
}

void naruto::Replication::_free_client(std::shared_ptr<narutoClient> & client) {
    if (!client->connect){
        client->connect->close();
    }

    if (_master && client->flags & (uint32_t)CLIENT_FLAG_MASTER){
        LOG(WARNING) << "Connect with master lost:" << client->remoteAddr();
        _repl_cache_master();
        _master = nullptr;
        _repl_state = state::CONNECT;
        _repl_down_since = std::chrono::steady_clock::now();
    }

    if (client->flags & (uint32_t)CLIENT_FLAG_SLAVE){
        LOG(WARNING) << "Connect with slave lost:" << client->remoteAddr();
        if (client->repl_state == state::SEND_BULK){
            if (client->repl_fd != -1) ::close(client->repl_fd);
        }
        if (_slaves.empty()){
            _repl_no_slaves_since = std::chrono::steady_clock::now();
        }
    }
}

void naruto::Replication::_repl_send_ack() {
    if (_master != nullptr){
        utils::Bytes pack;
        replication::command_ack ack;
        pack.putMessage(ack, COMMAND_REPL_ACK);
        ack.set_repl_off(_master->repl_off);
        _master->connect->send(pack);
    }
}

int naruto::Replication::_slave_try_partial_resynchronization() {
    // 设置为 -1 表示当时 run id 是不可用的
    _repl_master_initial_offset = -1;
    replication::command_psync psync;

    if (_cache_master){
        // 缓存存在，尝试部分重同步
        psync.set_repl_off(_cache_master->repl_off);
        psync.set_run_id(_cache_master->repl_run_id);
        LOG(INFO) << "Trying a partial resynchronization (request "
                <<  _cache_master->repl_run_id << ":" << _cache_master->repl_off << ").";
    }else{
        psync.set_repl_off(-1);
        psync.set_run_id(_cache_master->repl_run_id);
        LOG(INFO) << "Partial resynchronization not possible (no cached master)";
    }

    _master->sendMsg(psync, COMMAND_REPL_PSYNC);

    utils::Bytes pack;
    _master->connect->recv(pack);
    uint32_t len = pack.getInt();
    uint8_t version = pack.get();
    uint8_t flag = pack.get();
    uint16_t type = pack.getShort();

    switch (type){
        case COMMAND_REPL_FULLSYNC:
        {
            replication::command_fullsync fullsync;
            fullsync.ParseFromArray(&pack.data()[PACK_HEAD_LEN], len-PACK_HEAD_LEN);
            _master->repl_run_id = fullsync.run_id();
            _master->repl_off = fullsync.repl_off();
            _repl_master_initial_offset = fullsync.repl_off();
            LOG(INFO) << "Full resync from master "<< _master->repl_run_id << ":" << _master->repl_off;
            break;
        }
        case COMMAND_REPL_PARTSYNC:
        {
            replication::command_partsync partsync;
            partsync.ParseFromArray(&pack.data()[PACK_HEAD_LEN],len-PACK_HEAD_LEN);
            _master->repl_run_id = partsync.run_id();
            _master->repl_off = partsync.repl_off();
            _repl_master_initial_offset = partsync.repl_off();
            break;
        }
        default:
            LOG(INFO) << "Unexpected reply to PSYNC from master";
            return type;
    }

    _master->lastinteraction = std::chrono::steady_clock::now();
    _master->repl_state = state::CONNECTED;

    // 安装读事件
    _master->repl_ev_io_r = std::make_shared<ev::io>();
    _master->repl_ev_io_r->set<Replication, &Replication::onEvents>(this);
    _master->repl_ev_io_r->set(ev::get_default_loop());
    _master->repl_ev_io_r->start(_master->connect->fd(), ev::READ);

    // cache master 已经不需要了
    _repl_discard_cache_master();
    return type;
}

void naruto::Replication::_repl_cache_master() {
    assert(_master != nullptr && _cache_master);
    LOG(INFO) << "Caching the disconnected master state.";
    _cache_master = _master;
}

void naruto::Replication::_repl_discard_cache_master() {
    if (!_cache_master) return;
    LOG(INFO) << "Discarding previously cached master state.";
    _cache_master->flags &= ~CLIENT_FLAG_MASTER;
    _cache_master = nullptr;
}

naruto::Replication::~Replication() { }

const std::string &naruto::Replication::getMasterHost() const { return _master_host; }

void naruto::Replication::setMasterHost(const std::string &masterHost) { _master_host = masterHost; }

int naruto::Replication::getMasterPort() const { return _master_port; }

void naruto::Replication::setMasterPort(int masterPort) { _master_port = masterPort; }

bool naruto::Replication::isIsMaster() const { return _is_master; }

void naruto::Replication::setIsMaster(bool isMaster) { _is_master = isMaster; }
