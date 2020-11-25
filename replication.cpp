//
// Created by 王振奎 on 2020/9/18.
//


#include "replication.h"

#include <chrono>
#include <nlohmann/json.hpp>
#include <fcntl.h>
#include <thread>

#include "protocol/replication.pb.h"
#include "parameter/parameter_repl.h"
#include "utils/file.h"
#include "connect_worker.h"

naruto::Replication::Replication() {
    master_host_ = "";
    master_port_ = 0;
    dirty_ = 0;
    cronloops_ = 0;
    is_master_ = false;
    use_aof_checksum_ = true;
    bgsave_child_pid_ = -1;
    repl_timeout_sec_ = FLAGS_repl_timeout_sec;
    repl_pos_ = 0;
    repl_merge_size_ = worker_num;
    repl_command_incr_.store(0);
    back_log_.reserve(FLAGS_repl_back_log_size);
    master_repl_offset_ = 0;
    master_flush_repl_offset_ = 0;
    bgsave_last_status_ = 0;
    repl_ping_slave_period_ = (int)FLAGS_repl_ping_slave_period;
    repl_state_ = replState::NONE;
    repl_transfer_size_ = 0;
    repl_transfer_read_ = 0;
    repl_transfer_tmpfile_ = "";
    repl_transfer_lastio_ = std::chrono::system_clock::now();
    repl_slave_ro_ = false;
    repl_down_since_ = std::chrono::steady_clock::now();
    repl_master_runid_ = "";
    aof_off_before_bgsave_ = -1;
    saveparams_.push_back(saveparam{
        .ms=std::chrono::milliseconds(5),
        .changes=10,
    });

    saveparams_.push_back(saveparam{
            .ms=std::chrono::milliseconds(100),
            .changes=100,
    });

    aof_file_ = std::make_shared<sink::RotateFileStream>(FLAGS_repl_dir, FLAGS_repl_aof_rotate_size);
    _init_cronloops();
}

const std::string &naruto::Replication::getMasterHost() const { return master_host_; }

void naruto::Replication::setMasterHost(const std::string &masterHost) { master_host_ = masterHost; }

int naruto::Replication::getMasterPort() const { return master_port_; }

void naruto::Replication::setMasterPort(int masterPort) { master_port_ = masterPort; }

bool naruto::Replication::isIsMaster() const { return is_master_; }

void naruto::Replication::setIsMaster(bool isMaster) { is_master_ = isMaster; }

naruto::replState naruto::Replication::getReplState() const { return repl_state_; }

void naruto::Replication::setReplState(naruto::replState repl_state) { repl_state_ = repl_state; }


void naruto::Replication::_init_cronloops() {
    flush_watcher_.set<Replication, &Replication::onReplConfFlushCron>(this);
    flush_watcher_.set(ev::get_default_loop());
    flush_watcher_.start(FLAGS_repl_conf_flush_interval, FLAGS_repl_conf_flush_interval);

    cron_watcher_.set<Replication, &Replication::onReplCron>(this);
    cron_watcher_.set(ev::get_default_loop());
    cron_watcher_.start(FLAGS_repl_cron_interval, FLAGS_repl_cron_interval);

    bgsave_watcher_.set<Replication, &Replication::onBgsave>(this);
    bgsave_watcher_.set(ev::get_default_loop());
    bgsave_watcher_.start(FLAGS_repl_bgsave_interval, FLAGS_repl_bgsave_interval);
}

void naruto::Replication::_remove_bgsave_tmp_file(pid_t childpid) {
    std::string tmpfile = _bgsave_file_name(childpid);
    unlink(tmpfile.c_str());
}


void naruto::Replication::onReplCron(ev::timer& watcher, int event) {
    LOG(INFO) << "onReplCron...0";
    cronloops_++;
    repl_unixtime_ = steady_clock::now();
    auto repl_timeout_ms = repl_timeout_sec_ * 1000;
    // ====================== slave 情况 ===========================
    auto now = std::chrono::system_clock::now();
    auto last_transfer = duration_cast<milliseconds>(now - repl_transfer_lastio_);

    // 连接 master 超时
    if (!master_host_.empty()
        && (repl_state_ == replState::CONNECTING || repl_state_ == replState::RECEIVE_PONG)
        && last_transfer.count() > repl_timeout_ms){

        LOG(WARNING) <<  "Timeout connecting to the MASTER...";
        _undo_connect_master();
    }

    LOG(INFO) << "onReplCron...1";
    // dump db 传送超时
    if (!master_host_.empty() && repl_state_ == replState::TRANSFOR
        && last_transfer.count() > repl_timeout_ms){
        LOG(WARNING) << "Timeout receiving bulk data from MASTER... If the problem persists try to set the 'repl-timeout' parameter in naruto.conf to a larger value.";
        _abort_sync_transfer();
    }

    auto last_interaction = duration_cast<milliseconds>(now - repl_transfer_lastio_);
    LOG(INFO) << "onReplCron...2";
    // 从服务器曾经连接上主服务器，但现在超时
    if (!master_host_.empty() && repl_state_ == replState::CONNECTED &&
            last_interaction.count() > repl_timeout_ms){
        LOG(WARNING) <<"MASTER timeout: no data nor PING received...";
        workers[master_->worker_id].clientFree(master_);
    }

    LOG(INFO) << "onReplCron...3";
    // 尝试连接主服务器
    if (!master_host_.empty() && repl_state_ == replState::CONNECT){ _connect_master(); }
    LOG(INFO) << "onReplCron...4";

    // 定期向主服务器发送 ACK 命令
    if (!master_host_.empty() && master_ && repl_state_ == replState::CONNECTED){ _repl_send_ack(); }

    LOG(INFO) << "onReplCron...5";

    // ====================== master 情况 ===========================
    // 如果服务器有从服务器，定时向它们发送 PING 。
    // 这样服务器就可以实现显式的超时判断机制，
    // 即使 TCP 连接未断开也是如此。
    if ((cronloops_ % repl_ping_slave_period_) == 0 && !slaves_.empty()){
        LOG(INFO) << "onReplCron...5....1";
        replication::command_ping ping;
        ping.set_ip("127.0.0.1");
        for (const auto& slave : slaves_){
            slave->sendMsg(ping, replication::PING);
        }
    }

    LOG(INFO) << "onReplCron...6";
    // 断开超时从服务器
    auto it = slaves_.begin();
    while (it != slaves_.end()){
        if ((*it)->repl_state != replState::CONNECTED) continue;
        auto interval = duration_cast<milliseconds>(repl_unixtime_ - (*it)->lastinteraction);
        if (interval.count() > repl_timeout_ms){
            slaves_.erase(it);
            workers[master_->worker_id].clientFree(*it);
        }
    }

    LOG(INFO) << "onReplCron...7";
    _merge_backlog_feed_slaves();
}

// 定时把复制的一些持久状态保存到文件，重启时需要加载
void naruto::Replication::onReplConfFlushCron(ev::timer& watcher, int event) {
    if (master_ && !master_->repl_run_id.empty()){
        repl_conf_.master_run_id = master_->repl_run_id;
        repl_conf_.master_aof_filename = master_->repl_aof_file_name;
        repl_conf_.master_aof_off = master_->repl_aof_off;
        repl_conf_.is_master = false;
    } else {
        repl_conf_.is_master = true;
    }
    repl_conf_.toJson(FLAGS_repl_dir + "/" + FLAGS_repl_conf_filename);
}

void naruto::Replication::onBgsave(ev::timer& watcher, int event) {
    if (database::buckets->size() == 0){
        LOG(INFO) << "Bgsave database empty,give up bgsave.";
        return;
    }

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
        aof_filename_before_bgsave_ = curFile.name;
        aof_off_before_bgsave_ = curFile.offset;
        bgsave_fork_spends_ = std::chrono::duration_cast<std::chrono::milliseconds>(bgsave_time_start_ - bgsave_last_try_);
        bgsave_child_pid_ = childpid;
        LOG(INFO) << "Background saving started by pid " << childpid
                  << " bgsave aof filename " << curFile.name << " bgsave aof off " << curFile.offset;
    }
}

void naruto::Replication::onReadSyncBulkPayload(ev::io& watcher, int event) {
    char buf[4096];
    ssize_t nread, readlen;
    off_t left;
    left = repl_transfer_size_ - repl_transfer_read_;
    readlen = (left < (signed)sizeof(buf)) ? left : (signed)sizeof(buf);
    nread = ::read(watcher.fd, buf, readlen);
    if (nread <= 0){
        LOG(ERROR) << "I/O error trying to sync with MASTER: " << ((nread == -1) ? strerror(errno) : "connect lost");
        _abort_sync_transfer();
        return;
    }

    repl_transfer_lastio_ = std::chrono::system_clock::now();
    repl_transfer_f_->write(buf, nread);
    repl_transfer_read_ += nread;

    //检查 NDB 是否已经传送完毕
    if (repl_transfer_read_ == repl_transfer_size_){

        std::string dbname = FLAGS_repl_dir + "/" + FLAGS_repl_database_filename;
        if (::rename(repl_transfer_tmpfile_.c_str(), dbname.c_str()) == -1){
            LOG(INFO) << "Failed trying to rename the temp DB into naruto.db in MASTER <-> SLAVE synchronization: " << strerror(errno);
            _abort_sync_transfer();
            return;
        }
        repl_transfer_f_->close();
        repl_transfer_f_ = nullptr;
        repl_transfer_tmpfile_ = "";

        // 清空数据库
        database::buckets->flush();

        // 加载主服务同步过来的db文件
        LOG(INFO) << "MASTER <-> SLAVE sync: Flushing old data and Load new database";
        utils::File::loadFile(dbname,0,
                              [](uint16_t flag, uint16_t type, const unsigned char * s, size_t n)->void{
            database::buckets->parse(flag,type, s, n);
        });

        master_->flag = narutoClient::flags::MASTER;
        repl_state_ = replState::CONNECTED;

        // 安装读事件开始接受增量消息
        auto* r = new ev::io;
        r->set<naruto::narutoClient, &naruto::narutoClient::onRead>(master_.get());
        r->set(ev::get_default_loop());
        r->start(master_->connect->fd(), ev::READ);
        return;
    }
}

void naruto::Replication::onSyncWithMaster(ev::io & watcher, int event) {
    if (repl_state_ == replState::NONE) {
        ::close(watcher.fd);
        return;
    }
    int tryret, dfd, maxtries = 5;
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
    if (repl_state_ == replState::CONNECTING){
        LOG(INFO) << "Repl state CONNECTING, send PING to master";
        repl_state_ = replState::RECEIVE_PONG;
        repl_ev_io_w_->stop();

        replication::command_ping ping;
        ping.set_ip("1111");
        master_->sendMsg(ping, replication::PING);
        // 返回，等待 PONG 到达
        return;
    }

    if (repl_state_ == replState::RECEIVE_PONG){
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

    tryret = _slave_try_partial_resynchronization(watcher.fd);
    if (tryret == 0) return;

error:
    ::close(watcher.fd);
    if (repl_transfer_f_) repl_transfer_f_->close();
    repl_state_ = replState::CONNECT;
}

void naruto::Replication::onBgsaveFinish(ev::child& child, int events) {
    LOG(INFO) << "Replication::onBgsave.....0";
    if (bgsave_child_pid_ == -1) return;
    LOG(INFO) << "Replication::onBgsave.....1 bgsave_child_pid_:" << bgsave_child_pid_;

    if (child.pid == bgsave_child_pid_){
        std::string tempfile = _bgsave_file_name(child.pid);
        std::string db_name = FLAGS_repl_dir + "/" + FLAGS_repl_database_filename;
        if (::rename(tempfile.c_str(), db_name.c_str()) == -1){
            LOG(ERROR) << "Error trying to rename " << tempfile << " to " << db_name << " the temporary DB file: " << strerror(errno);
        }else{
            LOG(INFO) << "Background saving terminated with success";
            dirty_ = dirty_ - dirty_before_bgsave_;
            repl_conf_.aof_name = aof_filename_before_bgsave_;
            repl_conf_.aof_off = aof_off_before_bgsave_;
            bgsave_last_status_ = 0;
            auto now = std::chrono::steady_clock::now();
            bgsave_time_last_ = std::chrono::duration_cast<std::chrono::milliseconds>(now - bgsave_time_start_);
        }
    } else{
        if (child.pid == -1) LOG(INFO) << "Error bgsave callback: " << strerror(errno);
        _remove_bgsave_tmp_file(child.pid);
    }

    bgsave_child_pid_ = -1;
    child.stop();
    delete &child;
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
}

void naruto::Replication::bootStrap() {
    // 初始化一些值
    repl_state_ = replState::NONE;
    for (int i = 0; i < 2; ++i) {
        repl_[i] = std::make_shared<repl_workers>();
        repl_[i]->resize(worker_num);
    }

    // load database
    std::string dbname = FLAGS_repl_dir + "/" + FLAGS_repl_database_filename;
    utils::File::loadFile(dbname,0,
                          [](uint16_t flag, uint16_t type, const unsigned char * s, size_t n)->void{
        database::buckets->parse(flag,type, s, n);
    });

    // load repl conf
    std::string replname = FLAGS_repl_dir + "/" + FLAGS_repl_conf_filename;
    repl_conf_.fromJson(replname);

    LOG(INFO) << "Bootstrap repl conf:" << repl_conf_;

    // load aof
    std::vector<std::string> list;
    utils::File::listAof(FLAGS_repl_dir, list);
    for(const auto& filename : list){
        std::string aofname;
        aofname += FLAGS_repl_dir;
        aofname += "/";
        aofname += filename;
        utils::File::loadFile(aofname, repl_conf_.aof_off,
                              [](uint16_t flag, uint16_t type, const unsigned char* s, size_t n)->void{
            auto cmd = command::commands->fetch(type);
            if (cmd) cmd->execMsg(flag, type, s, n);
        });
    }
}

void naruto::Replication::_undo_connect_master() {
    assert(repl_state_ == replState::CONNECTING || repl_state_ == replState::RECEIVE_PONG);
    repl_ev_io_r_->stop();
    repl_ev_io_w_->stop();
    repl_state_ = replState::CONNECT;
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
        LOG(ERROR) << "Fail connecting to MASTER " << ops.host << ":" << ops.port << " errmsg:"
                   << master_->connect->errmsg();
        return;
    }

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
    repl_transfer_lastio_ = std::chrono::system_clock::now();
    repl_state_ = replState::CONNECTING;
}

// 停止下载 RDB 文件
void naruto::Replication::_abort_sync_transfer() {
    assert(repl_state_ == replState::TRANSFOR);
    repl_ev_io_r_->stop();
    if (repl_transfer_f_) repl_transfer_f_->close();
    ::unlink(repl_transfer_tmpfile_.c_str());
    repl_transfer_tmpfile_ = "";
    repl_state_ = replState::CONNECT;
}

void naruto::Replication::_repl_send_ack() {
    if (master_ != nullptr){
        replication::command_ack ack;
        ack.set_repl_aof_file_name(master_->repl_aof_file_name);
        ack.set_repl_aof_off(master_->repl_aof_off);
        master_->sendMsg(ack , replication::ACK);
    }
}

int naruto::Replication::_slave_try_partial_resynchronization(int fd) {
    replication::command_psync psync;
    if (!repl_conf_.master_run_id.empty()){
        // 缓存存在，尝试部分重同步
        psync.set_run_id(repl_conf_.master_run_id);
        psync.set_repl_aof_file_name(repl_conf_.master_aof_filename);
        psync.set_repl_aof_off(repl_conf_.master_aof_off);
        LOG(INFO) << "Trying a partial resynchronization (request "
                  << repl_conf_.master_run_id << ":" << repl_conf_.master_aof_filename << ":" << repl_conf_.master_aof_off << ").";
    }else{
        psync.set_run_id("?");
        psync.set_repl_aof_file_name("");
        psync.set_repl_aof_off(-1);
        LOG(INFO) << "Partial resynchronization not possible (no cached master)";
    }

    replication::command_psync_reply reply;
    uint16_t type;
    try {
        type = master_->sendMsg(psync, replication::PSYNC, reply);
    } catch (std::exception& e) {
        LOG(ERROR) << "Unexpected reply to PSYNC from master " << e.what();
        return -1;
    }

    if (reply.psync_type() == replication::FULLSYNC){

        LOG(INFO) << "MASTER <-> SLAVE sync: Master accepted a Full Resynchronization.";
        int maxtries = 5;
        master_->repl_run_id = reply.run_id();
        master_->repl_aof_file_name = reply.repl_aof_file_name();
        master_->repl_aof_off = reply.repl_aof_off();
        repl_transfer_size_ = reply.repl_database_size();
        repl_transfer_read_ = 0;
        repl_state_ = replState::TRANSFOR;

        // 安装读事件
        repl_ev_io_r_ = std::make_shared<ev::io>();
        repl_ev_io_r_->set<Replication, &Replication::onReadSyncBulkPayload>(this);
        repl_ev_io_r_->set(ev::get_default_loop());
        repl_ev_io_r_->start(master_->connect->fd(), ev::READ);

        // 打开一个临时文件用户存储接下来从主服务传来的 NDB 文件
        auto sec = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch());
        std::string tmpfile = FLAGS_repl_dir + "/tmp-" + std::to_string(sec.count()) + ".db";
        if (repl_transfer_f_) repl_transfer_f_->close();
        while (maxtries--){
            repl_transfer_f_ = std::make_shared<std::ofstream>(tmpfile, std::ios::out | std::ios::trunc | std::ios::binary);
            if (repl_transfer_f_->is_open()) break;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        repl_transfer_tmpfile_ = tmpfile;

    } else if (reply.psync_type() == replication::PARTSYNC){
        repl_state_ = replState::CONNECTED;
    }

    master_->lastinteraction = std::chrono::steady_clock::now();
    discardCacheMaster();
    return 0;
}

void naruto::Replication::cacheMaster() {
    assert(master_ != nullptr);
    LOG(INFO) << "Caching the disconnected master state.";
    repl_conf_.master_run_id = master_->repl_run_id;
    repl_conf_.master_aof_filename = master_->repl_aof_file_name;
    repl_conf_.master_aof_off = master_->repl_aof_off;
    repl_state_ = replState::CONNECT;
    repl_down_since_ = std::chrono::steady_clock::now();
    master_ = nullptr;
}

void naruto::Replication::statSlave() {
    if (slaves_.empty()){
        repl_no_slaves_since_ = std::chrono::system_clock::now();
    }
}

void naruto::Replication::discardCacheMaster() {
    LOG(INFO) << "Discarding previously cached master state.";
    repl_conf_.master_run_id = "";
    repl_conf_.master_aof_filename = "";
    repl_conf_.master_aof_off = -1;
}

std::string naruto::Replication::_bgsave_file_name(pid_t pid) {
    return FLAGS_repl_dir + "/tmp-" + std::to_string(pid) + ".db";
}

naruto::Replication::~Replication() { }

std::shared_ptr<naruto::Replication> naruto::replica = std::make_shared<naruto::Replication>();