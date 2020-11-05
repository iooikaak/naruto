//
// Created by 王振奎 on 2020/9/18.
//
#include <chrono>

#include "protocol/message_type.h"
#include "protocol/replication.pb.h"
#include "replication.h"
#include "parameter/parameter_repl.h"

naruto::Replication::Replication(std::shared_ptr<database::Buckets> buckets, int merge_threads_size) {
    master_host_ = "";
    master_port_ = 0;
    dirty_ = 0;
    cronloops_ = 0;
    is_master_ = false;
    use_aof_checksum_ = true;
    aof_child_pid_ = -1;
    repl_timeout_ = FLAGS_repl_timeout_sec;
    repl_pos_ = 0;
    repl_merge_size_ = merge_threads_size;
    repl_command_incr_.store(0);
    back_log_.reserve(0);
    master_repl_offset_ = 0;
    last_bgsave_status_ = 0;
    repl_ping_slave_period_ = (int)FLAGS_repl_ping_slave_period;
    repl_back_size_  = FLAGS_repl_back_log_size;
    repl_backlog_histlen_ = 0;
    repl_backlog_idx_ = 0;
    repl_backlog_off_ = 0;
    repl_state_ = state::NONE;
    repl_transfer_size_ = 0;
    repl_transfer_read_ = 0;
    repl_transfer_last_fsync_off_ = 0;
    repl_transfer_s_ = 0;
    repl_transfer_fd_ = 0;
    repl_transfer_tmpfile_ = "";
    repl_transfer_lastio_ = std::chrono::steady_clock::now();
    repl_slave_ro_ = false;
    repl_down_since_ = std::chrono::steady_clock::now();
    repl_master_runid_ = "";
    repl_master_initial_offset_ = 0;
    last_flush_backlog_off_ = 0;
    cron_interval_ = FLAGS_repl_cron_interval;
    buckets_ = buckets;
    bucket_num_ = buckets->size();
    time_watcher_.set<Replication, &Replication::onReplCron>(this);
    time_watcher_.set(ev::get_default_loop());
    time_watcher_.start(cron_interval_, cron_interval_);
}

void naruto::Replication::onReplCron(ev::timer &, int) {
    LOG(INFO) << "onReplCron...";
    // ====================== slave 情况 ===========================
    auto now = std::chrono::steady_clock::now();
    auto last_transfer = std::chrono::duration_cast<std::chrono::milliseconds>(now - repl_transfer_lastio_);
    // 连接 master 超时
    if (!master_host_.empty() && (repl_state_ == state::CONNECTING ||
                                  repl_state_ == state::RECEIVE_PONG) && last_transfer.count() > repl_timeout_){
        LOG(WARNING) <<  "Timeout connecting to the MASTER...";
        _undo_connect_master();
    }
    LOG(INFO) << "onReplCron...1";
    // dump db 传送超时
    if (!master_host_.empty() && repl_state_ == state::TRANSFOR
        && last_transfer.count() > repl_timeout_){
        LOG(WARNING) << "Timeout receiving bulk data from MASTER... If the problem persists try to set the 'repl-timeout' parameter in naruto.conf to a larger value.";
        _abort_sync_transfer();
    }

    auto last_interaction = std::chrono::duration_cast<std::chrono::milliseconds>(now - repl_transfer_lastio_);
    LOG(INFO) << "onReplCron...2";
    // 从服务器曾经连接上主服务器，但现在超时
    if (!master_host_.empty() && repl_state_ == state::CONNECTED &&
            last_interaction.count() > repl_timeout_){
        LOG(WARNING) <<"MASTER timeout: no data nor PING received...";
        _free_client(master_);
    }

    LOG(INFO) << "onReplCron...3";
    // 尝试连接主服务器
    if (!master_host_.empty() && repl_state_ == state::CONNECT){ _connect_master(); }
    LOG(INFO) << "onReplCron...4";
    // 定期向主服务器发送 ACK 命令
    if (!master_host_.empty() && master_ && repl_state_ == state::CONNECTED){ _repl_send_ack(); }

    LOG(INFO) << "onReplCron...5";
    // ====================== master 情况 ===========================
    // 如果服务器有从服务器，定时向它们发送 PING 。
    // 这样从服务器就可以实现显式的 master 超时判断机制，
    // 即使 TCP 连接未断开也是如此。
    if ((cronloops_ % repl_ping_slave_period_) == 0 && !slaves_.empty()){
        LOG(INFO) << "onReplCron...5....1";
        replication::command_ping ping;
        ping.set_ip("127.0.0.1");
        for (auto slave : slaves_){
            slave->sendMsg(ping, replication::PING);
        }
    }

    LOG(INFO) << "onReplCron...6";
    // 断开超时从服务器
    auto it = slaves_.begin();
    while (it != slaves_.end()){
        if ((*it)->repl_state != state::ONLINE) continue;
        auto interval = std::chrono::duration_cast<std::chrono::milliseconds>(repl_unixtime_ - (*it)->repl_ack_time);
        if (interval.count() > repl_timeout_){
            slaves_.erase(it);
            _free_client((*it));
        }
    }

    LOG(INFO) << "onReplCron...7";
    _merge_backlog_feed_slaves();
}

void naruto::Replication::onReadSyncBulkPayload(ev::io& watcher, int event) {

}

void naruto::Replication::onSyncWithMaster(ev::io & watcher, int event) {
    if (repl_state_ == state::NONE) {
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
        repl_ev_io_w_->stop();
        repl_ev_io_r_->stop();
        LOG(WARNING) << "Error condition on socket for SYNC: " << strerror(sockerr);
        goto error;
    }

    // 如果状态为 CONNECTING ，那么在进行初次同步之前，
    // 向主服务器发送一个非阻塞的 PING
    // 因为接下来的 RDB 文件发送非常耗时，所以我们想确认主服务器真的能访问
    if (repl_state_ == state::CONNECTING){
        LOG(INFO) << "Repl state CONNECTING, send PING to master";
        repl_state_ = state::RECEIVE_PONG;
        repl_ev_io_w_->stop();

        replication::command_ping ping;
        ping.set_ip("1111");
        master_->sendMsg(ping, replication::PING);
        // 返回，等待 PONG 到达
        return;
    }

    if (repl_state_ == state::RECEIVE_PONG){
        LOG(INFO) << "Repl state RECEIVE_PONG";
        repl_ev_io_r_->stop();
        replication::command_pong pong;
        uint16_t type = master_->read(pong);
        LOG(INFO) << "Repl state RECEIVE_PONG, master_->read";
        if (type != replication::PONG){
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
    repl_transfer_fd_ = -1;
    repl_state_ = state::CONNECT;
    return;
}

void naruto::Replication::backlogFeed(int tid, const ::google::protobuf::Message & msg, uint16_t type) {
    auto repl = repl_[repl_pos_.load()];

    // 写到复制积压缓冲区
    auto& local_repl = (*repl)[tid];
    auto command_id = repl_command_incr_.load();
    repl_command_incr_++;
    // 把 repl_command_incr_ 写在包的前面用于后续多路归并，八字节
    local_repl.putLong(command_id);
    utils::Pack::serialize(msg, type, local_repl);
}

void naruto::Replication::backlogFeed(int tid, utils::Bytes& pack) {
    auto repl = repl_[repl_pos_.load()];
    // 写到复制积压缓冲区
    auto& local_repl = (*repl)[tid];
    auto command_id = repl_command_incr_.load();
    repl_command_incr_++;
    // 把 repl_command_incr_ 写在包的前面用于后续多路归并，八字节
    local_repl.putLong(command_id);
    local_repl.put(pack);
}

void naruto::Replication::backgroundSave() {
    if (aof_child_pid_ != -1) return;

    lastbgsave_try_ = std::chrono::steady_clock::now();
    pid_t childpid;

    if ((childpid = fork()) == 0){ // child
        char tmpfile[256];
        std::snprintf(tmpfile, 256, "tmp-%d.aof",(int)getpid());
        int ret = buckets_->dump(tmpfile);
        _exit(ret);
//        setproctitle("%s %s:%d%s",
//                     "naruto-aof-bgsave",
//                     "",
//                     _port,
//                     server_mode);
    } else {
        stat_fork_spends_ = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - lastbgsave_try_);
        if (childpid == -1){
            last_bgsave_status_ = -1;
            LOG(INFO) << "Can't save in background, fork: " << strerror(errno);
            return;
        }
        LOG(INFO) << "Background saving started by pid " << childpid;
        aof_save_time_start_ = std::chrono::steady_clock::now();
        aof_child_pid_ = childpid;
        last_bgsave_status_ = 0;
    }
}

void naruto::Replication::_backlog_feed(const unsigned char* data, size_t size) {
    dirty_++;
    auto* ptr = const_cast<unsigned char *>(data);
    size_t len = size;
    master_repl_offset_ += (long long)len;
    while (len){
        // 从 idx 到 backlog 尾部的字节数
        size_t thislen = repl_back_size_ - repl_backlog_idx_;
        // 如果 idx 到 backlog 尾部这段空间足以容纳要写入的内容
        // 那么直接将写入数据长度设为 len
        // 在将这些 len 字节复制之后，这个 while 循环将跳出
        if (thislen > len) thislen = len;
        memcpy((char*)&back_log_[repl_backlog_idx_], ptr, thislen);

        // 更新 idx ，指向新写入的数据之后
        repl_backlog_idx_ += thislen;
        // 如果写入达到尾部，那么将索引重置到头部
        if (repl_backlog_idx_ == repl_back_size_)
            repl_backlog_idx_ = 0;
        len -= thislen;
        // 将指针移动到已被写入数据的后面，指向未被复制数据的开头
        ptr += thislen;
        repl_backlog_histlen_ += thislen;
    }

    // histlen 的最大值只能等于 backlog_size
    // 另外，当 histlen 大于 repl_backlog_size 时，
    // 表示写入数据的前头有一部分数据被自己的尾部覆盖了
    if (repl_backlog_histlen_ > repl_back_size_)
        repl_backlog_histlen_ = repl_back_size_;

    // 记录程序可以依靠 backlog 来还原的数据的第一个字节的偏移量
    // 比如 master_repl_offset = 10086
    // repl_backlog_histlen = 30
    // 那么 backlog 所保存的数据的第一个字节的偏移量为
    // 10086 - 30 + 1 = 10056 + 1 = 10057
    // 这说明如果从服务器如果从 10057 至 10086 之间的任何时间断线
    // 那么从服务器都可以使用 PSYNC
    // repl_backlog_off_ + 1 即为可部分重同步的 index
    repl_backlog_off_ = master_repl_offset_ - repl_backlog_histlen_;

    // 判断是否将 aof 刷到文件
    auto interval = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - last_save_aof_time_);
    bool is_save = false;
    for (auto condition : saveparams_){
        if (interval > condition.ms && dirty_ > condition.changes){
            is_save = true;
            break;
        }
    }
    // aof flush
    if (is_save){ _backlog_flush(); }
}

void naruto::Replication::_backlog_flush() {
    std::ofstream out(aof_file_name_, std::ofstream::binary|std::ofstream::app|std::ofstream::out);
    long long k, skip, flush_len;
    skip = last_flush_backlog_off_ - repl_backlog_off_;
    k = (repl_backlog_idx_ + (repl_back_size_ - repl_backlog_histlen_)) % repl_back_size_;
    k = (k + skip) % repl_back_size_;
    flush_len = repl_backlog_histlen_ - skip;
    while (flush_len){
        long long thislen = ((repl_back_size_ - k) < flush_len) ? (repl_back_size_ - k) : flush_len;
        out.write((const char*)&back_log_[k], thislen);
        flush_len -= thislen;
        k = 0;
    }
    out.close();
    last_flush_backlog_off_ += flush_len;
    dirty_ = 0;
}

void naruto::Replication::_merge_backlog_feed_slaves() {
    auto cur_pos = repl_pos_.load();
    auto next_pos = 1 - cur_pos;
    for (int i = 0; i < repl_merge_size_; ++i) {
        repl_[next_pos]->at(i).clear();
    }
    repl_pos_.store(next_pos);

    // 多路归并
    auto repl_data = repl_[cur_pos];
    std::vector<int> p(repl_merge_size_);
    std::vector<indexLog> id_list(repl_merge_size_);

    for (int j = 0; j < repl_merge_size_; ++j) {
        if (p[j] < repl_data->at(j).size()){
            id_list.push_back(indexLog{
                    .tid = j,
                    .id = repl_data->at(j).getLong(p[j]),
            });
            p.at(j) += sizeof(uint64_t);
        }
    }

    while (!id_list.empty()){
        std::sort(id_list.begin(), id_list.end(), [](const indexLog& l, const indexLog& r){
            return l.id < r.id;
        });

        // 找到最小的
        auto min_i = id_list.at(0).tid;
        auto pos = p[min_i];
        // 读取 min_i bucket 一个包
        auto& min_bucket = repl_data->at(min_i);
        auto pack_size = min_bucket.getLong(pos);

        // merge 到 全局 backlog 中
        // 需要去掉包头
        _backlog_feed(&min_bucket.data()[pos + PACK_HEAD_LEN],pack_size - PACK_HEAD_LEN);
        p.at(min_i) += pack_size;

        // 删除最小的一个
        id_list.erase(id_list.begin());

        // 最小的那个 p[min_i] 指向下一个位置
        if (p[min_i] < repl_data->at(min_i).size()){
            id_list.push_back(indexLog{
                    .tid = min_i,
                    .id = repl_data->at(min_i).getLong(p[min_i]),
            });
            p.at(min_i) += sizeof(uint64_t);
        }
    }

    // 发送给所有的 slaves
    for (const auto& slave : slaves_){
        // 不要给正在等待 BGSAVE 开始的从服务器发送命令
        if (slave->repl_state == state::WAIT_BGSAVE_START) continue;
        // 向已经接收完 RDB 文件的从服务器发送命令
        try {
//            slave->connect->send(pack);
        }catch (std::exception& e){
            LOG(ERROR) << "Feed slave:" << e.what();
        }
    }
}

void naruto::Replication::_undo_connect_master() {
    int fd = repl_transfer_s_;
    assert(repl_state_ == state::CONNECTING || repl_state_ == state::RECEIVE_PONG);
    repl_ev_io_r_->stop();
    repl_ev_io_w_->stop();
    ::close(fd);
    repl_transfer_s_ = -1;
    repl_state_ = state::CONNECT;
}

void naruto::Replication::_connect_master() {
    connection::ConnectOptions ops;
    ops.host = master_host_;
    ops.port = master_port_;

    LOG(ERROR) << "Try connecting to MASTER "<< ops.host << ":" << ops.port;
    // 连接并监听主服务的读写事件
    master_ = std::make_shared<narutoClient>();
    master_->connect = std::make_shared<naruto::connection::Connect>(ops);

    if (master_->connect->connect() != CONNECT_RT_OK){
        LOG(ERROR) << "Fail connecting to MASTER " << ops.host << ":" << ops.port << ",errmsg:"
                   << master_->connect->errmsg();
        return;
    }

    master_->flags |= (uint32_t)CLIENT_FLAG_MASTER;
    LOG(INFO) << "MASTER <-> SLAVE sync started";
    repl_ev_io_r_ = std::make_shared<ev::io>();
    repl_ev_io_r_->set<Replication, &Replication::onSyncWithMaster>(this);
    repl_ev_io_r_->set(ev::get_default_loop());
    repl_ev_io_r_->start(master_->connect->fd(), ev::READ);

    repl_ev_io_w_ = std::make_shared<ev::io>();
    repl_ev_io_w_->set<Replication, &Replication::onSyncWithMaster>(this);
    repl_ev_io_w_->set(ev::get_default_loop());
    repl_ev_io_w_->start(master_->connect->fd(), ev::WRITE);

    // 初始化统计变量
    repl_transfer_lastio_ = std::chrono::steady_clock::now();
    repl_transfer_fd_ = master_->connect->fd();
    repl_state_ = state::CONNECTING;
}

// 停止下载 RDB 文件
void naruto::Replication::_abort_sync_transfer() {
    assert(repl_state_ == state::TRANSFOR);
    repl_ev_io_r_->stop();
    ::close(repl_transfer_fd_);
    ::unlink(repl_transfer_tmpfile_.c_str());
    repl_transfer_tmpfile_ = "";
    repl_state_ = state::CONNECT;
}

void naruto::Replication::_free_client(std::shared_ptr<narutoClient> & client) {
    if (!client->connect){
        client->connect->close();
    }

    if (master_ && client->flags & (uint32_t)CLIENT_FLAG_MASTER){
        LOG(WARNING) << "Connect with master lost:" << client->remoteAddr();
        _repl_cache_master();
        master_ = nullptr;
        repl_state_ = state::CONNECT;
        repl_down_since_ = std::chrono::steady_clock::now();
    }

    if (client->flags & (uint32_t)CLIENT_FLAG_SLAVE){
        LOG(WARNING) << "Connect with slave lost:" << client->remoteAddr();
        if (client->repl_state == state::SEND_BULK){
            if (client->repl_fd != -1) ::close(client->repl_fd);
        }
        if (slaves_.empty()){
            repl_no_slaves_since_ = std::chrono::steady_clock::now();
        }
    }
}

void naruto::Replication::_repl_send_ack() {
    if (master_ != nullptr){
        replication::command_ack ack;
        ack.set_repl_off(master_->repl_off);
        master_->sendMsg(ack , replication::ACK);
    }
}

int naruto::Replication::_slave_try_partial_resynchronization() {
    // 设置为 -1 表示当时 run id 是不可用的
    repl_master_initial_offset_ = -1;
    replication::command_psync psync;

    if (cache_master_){
        // 缓存存在，尝试部分重同步
        psync.set_repl_off(cache_master_->repl_off);
        psync.set_run_id(cache_master_->repl_run_id);
        LOG(INFO) << "Trying a partial resynchronization (request "
                  << cache_master_->repl_run_id << ":" << cache_master_->repl_off << ").";
    }else{
        psync.set_repl_off(-1);
        psync.set_run_id("?");
        LOG(INFO) << "Partial resynchronization not possible (no cached master)";
    }

    master_->write(psync, replication::PSYNC);
    utils::Bytes pack;
    master_->connect->recv(pack);
    
    uint16_t type = pack.getShort();
    switch (type){
        case replication::FULLSYNC :
        {
            replication::command_fullsync fullsync;
            utils::Pack::deSerialize(pack, fullsync);

            master_->repl_run_id = fullsync.run_id();
            master_->repl_off = fullsync.repl_off();
            repl_master_initial_offset_ = fullsync.repl_off();
            LOG(INFO) << "Full resync from master " << master_->repl_run_id << ":" << master_->repl_off;
            break;
        }
        case replication::PARTSYNC:
        {
            replication::command_partsync partsync;
            utils::Pack::deSerialize(pack, partsync);

            master_->repl_run_id = partsync.run_id();
            master_->repl_off = partsync.repl_off();
            repl_master_initial_offset_ = partsync.repl_off();
            break;
        }
        default:
            LOG(INFO) << "Unexpected reply to PSYNC from master";
            return type;
    }

    master_->lastinteraction = std::chrono::steady_clock::now();
    master_->repl_state = state::CONNECTED;

    // 安装读事件
    master_->repl_ev_io_r = std::make_shared<ev::io>();
    master_->repl_ev_io_r->set<Replication, &Replication::onReadSyncBulkPayload>(this);
    master_->repl_ev_io_r->set(ev::get_default_loop());
    master_->repl_ev_io_r->start(master_->connect->fd(), ev::READ);

    // cache master 已经不需要了
    _repl_discard_cache_master();
    return type;
}

void naruto::Replication::_repl_cache_master() {
    assert(master_ != nullptr && cache_master_);
    LOG(INFO) << "Caching the disconnected master state.";
    cache_master_ = master_;
}

void naruto::Replication::_repl_discard_cache_master() {
    if (!cache_master_) return;
    LOG(INFO) << "Discarding previously cached master state.";
    cache_master_->flags &= ~CLIENT_FLAG_MASTER;
    cache_master_ = nullptr;
}

naruto::Replication::~Replication() { }

const std::string &naruto::Replication::getMasterHost() const { return master_host_; }

void naruto::Replication::setMasterHost(const std::string &masterHost) { master_host_ = masterHost; }

int naruto::Replication::getMasterPort() const { return master_port_; }

void naruto::Replication::setMasterPort(int masterPort) { master_port_ = masterPort; }

bool naruto::Replication::isIsMaster() const { return is_master_; }

void naruto::Replication::setIsMaster(bool isMaster) { is_master_ = isMaster; }

naruto::state naruto::Replication::getReplState() const { return repl_state_; }

void naruto::Replication::setReplState(naruto::state repl_state) { repl_state_ = repl_state; }
