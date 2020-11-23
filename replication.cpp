//
// Created by 王振奎 on 2020/9/18.
//


#include "replication.h"

#include <chrono>
#include "protocol/message_type.h"
#include "protocol/replication.pb.h"
#include "parameter/parameter_repl.h"

naruto::Replication::Replication(int worker_num) {
    master_host_ = "";
    master_port_ = 0;
    dirty_ = 0;
    cronloops_ = 0;
    is_master_ = false;
    use_aof_checksum_ = true;
    bgsave_child_pid_ = -1;
    repl_timeout_ = FLAGS_repl_timeout_sec;
    repl_pos_ = 0;
    repl_merge_size_ = worker_num;
    repl_command_incr_.store(0);
    back_log_.reserve(FLAGS_repl_back_log_size);
    master_repl_offset_ = 0;
    master_flush_repl_offset_ = 0;
    bgsave_last_status_ = 0;
    repl_ping_slave_period_ = (int)FLAGS_repl_ping_slave_period;
    repl_back_size_  = FLAGS_repl_back_log_size;
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
    cron_run_ = false;
    saveparams_.push_back(saveparam{
        .ms=std::chrono::milliseconds(5),
        .changes=10,
    });

    saveparams_.push_back(saveparam{
            .ms=std::chrono::milliseconds(100),
            .changes=100,
    });

    for (int i = 0; i < 2; ++i) {
        repl_[i] = std::make_shared<repl_workers>();
        repl_[i]->resize(repl_merge_size_);
    }
    cron_interval_ = FLAGS_repl_cron_interval;
    aof_file_ = std::make_shared<sink::RotateFileStream>(FLAGS_repl_dir, FLAGS_repl_aof_rotate_size);
}

void naruto::Replication::_remove_bgsave_tmp_file(pid_t childpid) {
    std::string tmpfile = _bgsave_file_name(childpid);
    unlink(tmpfile.c_str());
}

void naruto::Replication::onReplCron() {
    if (cron_run_) return;
    cron_run_ = true;
//    LOG(INFO) << "onReplCron...";
    // ====================== slave 情况 ===========================
    auto now = std::chrono::steady_clock::now();
    auto last_transfer = std::chrono::duration_cast<std::chrono::milliseconds>(now - repl_transfer_lastio_);
    // 连接 master 超时
    if (!master_host_.empty() && (repl_state_ == state::CONNECTING ||
                                  repl_state_ == state::RECEIVE_PONG) && last_transfer.count() > repl_timeout_){
        LOG(WARNING) <<  "Timeout connecting to the MASTER...";
        _undo_connect_master();
    }
//    LOG(INFO) << "onReplCron...1";
    // dump db 传送超时
    if (!master_host_.empty() && repl_state_ == state::TRANSFOR
        && last_transfer.count() > repl_timeout_){
        LOG(WARNING) << "Timeout receiving bulk data from MASTER... If the problem persists try to set the 'repl-timeout' parameter in naruto.conf to a larger value.";
        _abort_sync_transfer();
    }

    auto last_interaction = std::chrono::duration_cast<std::chrono::milliseconds>(now - repl_transfer_lastio_);
//    LOG(INFO) << "onReplCron...2";
    // 从服务器曾经连接上主服务器，但现在超时
    if (!master_host_.empty() && repl_state_ == state::CONNECTED &&
            last_interaction.count() > repl_timeout_){
        LOG(WARNING) <<"MASTER timeout: no data nor PING received...";
        _free_client(master_);
    }

//    LOG(INFO) << "onReplCron...3";
    // 尝试连接主服务器
    if (!master_host_.empty() && repl_state_ == state::CONNECT){ _connect_master(); }
//    LOG(INFO) << "onReplCron...4";
    // 定期向主服务器发送 ACK 命令
    if (!master_host_.empty() && master_ && repl_state_ == state::CONNECTED){ _repl_send_ack(); }

//    LOG(INFO) << "onReplCron...5";
    // ====================== master 情况 ===========================
    // 如果服务器有从服务器，定时向它们发送 PING 。
    // 这样从服务器就可以实现显式的 master 超时判断机制，
    // 即使 TCP 连接未断开也是如此。
    if ((cronloops_ % repl_ping_slave_period_) == 0 && !slaves_.empty()){
        LOG(INFO) << "onReplCron...5....1";
        replication::command_ping ping;
        ping.set_ip("127.0.0.1");
        for (const auto& slave : slaves_){
            slave->sendMsg(ping, replication::PING);
        }
    }

//    LOG(INFO) << "onReplCron...6";
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

//    LOG(INFO) << "onReplCron...7";
    _merge_backlog_feed_slaves();
    cron_run_ = false;
}

// 定时把复制的一些持久状态保存到文件，重启时需要加载
void naruto::Replication::onReplConfFlush() {

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

    type = _slave_try_partial_resynchronization(watcher.fd);
    if (type == replication::PARTSYNC) {
        LOG(INFO) << "MASTER <-> SLAVE sync: Master accepted a Partial Resynchronization.";
        return;
    }else if (type == replication::FULLSYNC){
        LOG(INFO) << "MASTER <-> SLAVE sync: Master accepted a Full Resynchronization.";
        return;
    }

    // 走到这则type不可识别
error:
    ::close(watcher.fd);
    repl_transfer_fd_ = -1;
    repl_state_ = state::CONNECT;
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

void naruto::Replication::onBgsaveFinish(ev::child& child, int events) {
    LOG(INFO) << "Replication::onBgsave.....0";
    if (bgsave_child_pid_ == -1) return;
    LOG(INFO) << "Replication::onBgsave.....1 bgsave_child_pid_:" << bgsave_child_pid_;

    if (child.pid == bgsave_child_pid_){
        LOG(INFO) << "Background saving terminated with success";
        dirty_ = dirty_ - dirty_before_bgsave_;
        bgsave_last_status_ = 0;
        auto now = std::chrono::steady_clock::now();
        bgsave_time_last_ = std::chrono::duration_cast<std::chrono::milliseconds>(now - bgsave_time_start_);
        std::string tempfile = _bgsave_file_name(child.pid);

        if (::rename(tempfile.c_str(), DATABASE_NAME) == -1){
            LOG(ERROR) << "Error trying to rename the temporary DB file: " << strerror(errno);
        }
    } else{
        if (child.pid == -1) LOG(INFO) << "Error bgsave callback: " << strerror(errno);
        _remove_bgsave_tmp_file(child.pid);
    }

    bgsave_child_pid_ = -1;
    child.stop();
    delete &child;
}

void naruto::Replication::databaseLoad() {
    // load database
    database::buckets->load(FLAGS_repl_dir + "/" + FLAGS_repl_database_filename);
    // load aof
    std::vector<std::string> list;
    sink::RotateFileStream::listAof(FLAGS_repl_dir, list);
    for(const auto& filename : list){

    }
}

void naruto::Replication::bgsave() {
    if (bgsave_child_pid_ != -1) return;
    bgsave_last_try_ = std::chrono::steady_clock::now();
    pid_t childpid;
    if ((childpid = ::fork()) == 0){ // child
        std::string tmpfile = _bgsave_file_name(getpid());
        int ret = database::buckets->dump(tmpfile);
        sleep(1);
        _exit(ret);
    } else {
        if (childpid == -1){
            bgsave_last_status_ = -1;
            LOG(INFO) << "Can't save in background, fork: " << strerror(errno);
            return;
        }

        auto* childw = new ev::child;
        childw->set(ev::get_default_loop());
        childw->set<Replication, &Replication::onBgsaveFinish>(this);
        childw->start(childpid);

        bgsave_time_start_ = std::chrono::steady_clock::now();
        dirty_before_bgsave_ = dirty_;
        auto curFile = aof_file_->curRollFile();
        bgsave_aof_filename_ = curFile.name;
        bgsave_aof_off = curFile.offset;
        bgsave_fork_spends_ = std::chrono::duration_cast<std::chrono::milliseconds>(bgsave_time_start_ - bgsave_last_try_);
        bgsave_child_pid_ = childpid;
        bgsave_last_status_ = 0;
        LOG(INFO) << "Background saving started by pid " << childpid
                    << " bgsave aof filename " << bgsave_aof_filename_ << " bgsave aof off " << bgsave_aof_off;
    }
}

void naruto::Replication::_backlog_feed(const unsigned char* data, size_t size) {
//    client::command_hget_int cmd;
//    utils::Pack::deSerialize(data, size, cmd);
//
//    LOG(INFO) << "_backlog_feed:" << cmd.DebugString();
    changes_++;
    dirty_++;
    auto* ptr = const_cast<unsigned char *>(data);
    size_t len = size;
    master_repl_offset_ += (long long)len;

    while (len){
        back_log_.push_back(*ptr);
        ptr++;
        len--;
    }

    // 判断是否将 aof 刷到文件
    auto interval = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - last_flush_aof_time_);
    bool is_save = false;
    for (auto condition : saveparams_){
        if (interval > condition.ms && dirty_ > condition.changes){
            is_save = true;
            break;
        }
    }
//    if (changes_ == 100000){
//        LOG(INFO) << " changes_ flush spends" << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - repl_transfer_lastio_).count() << "ms";
//    }
    // aof flush
    if (is_save){ _backlog_flush(); }
}

void naruto::Replication::_backlog_flush() {
    aof_file_->write((const char*)&back_log_[0], back_log_.size());
    master_flush_repl_offset_ += back_log_.size();
    last_flush_aof_time_ = std::chrono::steady_clock::now();
    back_log_.clear();
    dirty_ = 0;
}

void naruto::Replication::_merge_backlog_feed_slaves() {
//    LOG(INFO) << "_merge_backlog_feed_slaves...1";
    auto cur_pos = repl_pos_.load();
    auto next_pos = 1 - cur_pos;
    for (int i = 0; i < repl_merge_size_; ++i) {
        repl_[next_pos]->at(i).clear();
    }
    repl_pos_.store(next_pos);
    // 多路归并
    auto repl_data = repl_[cur_pos];
    std::vector<int> p(repl_merge_size_);
    std::vector<indexLog> id_list;
    id_list.reserve(repl_merge_size_);

    for (int j = 0; j < repl_merge_size_; ++j) {
        if (p[j] < repl_data->at(j).size()){
            id_list.push_back(indexLog{
                    .tid = j,
                    .id = repl_data->at(j).getLong(p[j]),
            });
            p.at(j) += sizeof(uint64_t);
        }
    }

//    LOG(INFO) << "_merge_backlog_feed_slaves...2";
    while (!id_list.empty()){
        std::sort(id_list.begin(), id_list.end(), [](const indexLog& l, const indexLog& r){
            return l.id < r.id;
        });
//        for (int i = 0; i < repl_merge_size_; ++i) {
//            LOG(INFO) << "id_list[" << i << "].tid=" << id_list[i].tid << " id=" << id_list[i].id;
//        }
        // 找到最小的
        auto min_i = id_list.at(0).tid;
        auto pos = p[min_i];
        // 读取 min_i bucket 一个包
        auto& min_bucket = repl_data->at(min_i);
        auto pack_size = min_bucket.getInt(pos);
        // merge 到 全局 backlog 中
        _backlog_feed(&min_bucket.data()[pos],pack_size);
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

//    LOG(INFO) << "_merge_backlog_feed_slaves...3";
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
            client->connect->close();
        }
        if (slaves_.empty()){
            repl_no_slaves_since_ = std::chrono::steady_clock::now();
        }
    }
}

void naruto::Replication::_repl_send_ack() {
    if (master_ != nullptr){
        replication::command_ack ack;
        ack.set_repl_aof_file_name(master_->repl_aof_file_name);
        ack.set_repl_aof_off(master_->repl_aof_off);
        master_->sendMsg(ack , replication::ACK);
    }
}

replication::type naruto::Replication::_slave_try_partial_resynchronization(int fd) {
    replication::command_psync psync;
    if (cache_master_){
        // 缓存存在，尝试部分重同步
        psync.set_run_id(cache_master_->repl_run_id);
        psync.set_repl_aof_file_name(cache_master_->repl_aof_file_name);
        psync.set_repl_aof_off(cache_master_->repl_aof_off);
        LOG(INFO) << "Trying a partial resynchronization (request "
                  << cache_master_->repl_run_id << ":" << cache_master_->repl_aof_file_name << ":" << cache_master_->repl_aof_off << ").";
    }else{
        psync.set_run_id("?");
        psync.set_repl_aof_file_name("");
        psync.set_repl_aof_off(-1);
        LOG(INFO) << "Partial resynchronization not possible (no cached master)";
    }

    replication::command_psync_reply reply;
    uint16_t type = master_->sendMsg(psync, replication::PSYNC, reply);
    if (type != replication::PSYNC || reply.errcode() != 0){
        LOG(ERROR) << "Unexpected reply to PSYNC from master,type:" << type << " errmsg:" << reply.errmsg();
        return replication::TYPE_NULL;
    }

    if (reply.psync_type() == replication::FULLSYNC){
        master_->repl_run_id = reply.run_id();
        master_->repl_aof_file_name = reply.repl_aof_file_name();
        master_->repl_aof_off = reply.repl_aof_off();
        repl_database_size = reply.repl_database_size();
        repl_database_synced_size = 0;
        // 安装读事件
        repl_ev_io_r_ = std::make_shared<ev::io>();
        repl_ev_io_r_->set<Replication, &Replication::onReadSyncBulkPayload>(this);
        repl_ev_io_r_->set(ev::get_default_loop());
        repl_ev_io_r_->start(master_->connect->fd(), ev::READ);
        _repl_discard_cache_master(); // cache master 已经不需要了

    } else if (reply.psync_type() == replication::PARTSYNC){
        if (cache_master_->connect) cache_master_->connect->close();
        master_ = cache_master_;
        master_->connect = std::make_shared<connection::Connect>(fd);
    }

    master_->lastinteraction = std::chrono::steady_clock::now();
    repl_state_ = state::CONNECTED;
    return reply.psync_type();
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
    _free_client(cache_master_);
    cache_master_ = nullptr;
}

std::string naruto::Replication::_bgsave_file_name(pid_t pid) {
    return FLAGS_repl_dir + "/tmp-" + std::to_string(pid) + ".db";
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
