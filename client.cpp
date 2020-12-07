//
// Created by 王振奎 on 2020/9/27.
//

#include "client.h"

#include "global.h"
#include "connect_worker.h"
#include "replication.h"
#include "command/commands.h"
#include "protocol/client.pb.h"
#include "protocol/command_types.pb.h"
#include "defines.h"
#include "utils/file.h"

DECLARE_string(repl_dir);
DECLARE_double(repl_aof_interval);

naruto::narutoClient::narutoClient() {
    flag |=(unsigned)flags::NONE;
    worker_id = -1;
    repl_state = replState::NONE;
    repl_run_id = "";
    repl_aof_file_name = "";
    repl_aof_off = -1;
    repl_dboff = -1;
    repl_dbsize = -1;
    ctime = std::chrono::system_clock::now();
    repl_ack_time = std::chrono::steady_clock::now();
    lastinteraction = std::chrono::steady_clock::now();
}

void naruto::narutoClient::onRead(ev::io &watcher, int events) {
    lastinteraction = std::chrono::steady_clock::now();
    if (ev::ERROR & events){
        LOG(ERROR) << "connect worker error :" << strerror(errno);
        return;
    }
    LOG(INFO) << "onRead------>> worker_id=" << worker_id << " step 0";

    try {
        rbuf.clear();
        connect->recv(rbuf);
    }catch (utils::Error& e){
        close();
        return;
    }

    uint32_t pack_size = rbuf.getInt();
    uint16_t fg = rbuf.getShort();
    uint16_t type = rbuf.getShort();

    LOG(INFO) << "onRead------>> workid:"<< worker_id
                << " recv:" << remoteAddr()
                << " pack_size:" << pack_size
                << " flag:"<< (unsigned)fg
                << " type:" << client::Type_descriptor()->FindValueByNumber(type)->name();

    command::commands->fetch(type)->exec(this);
}

void naruto::narutoClient::onSendBulkToSlave(ev::io& watcher, int event) {
    if (!repl_db_f) return;
    LOG(INFO) << "Master send bulk to slave...";
    char buf[TRANSFER_BUF_LEN];
    ssize_t nwritten, buflen;

    repl_db_f->seekg(repl_dboff, std::ios::beg);
    repl_db_f->read(buf, TRANSFER_BUF_LEN);

    if ((buflen = repl_db_f->gcount()) <= 0){
        LOG(ERROR) << "Read error sending DB to slave: " << ((buflen == 0) ? "premature EOF" : strerror(errno));
        close();
        return;
    }

    if ((nwritten = ::write(connect->fd(), buf, buflen)) == -1){
        if (errno != EAGAIN){
            LOG(WARNING) << "Write error sending DB to slave: " << strerror(errno);
            close();
        }
        return;
    }

    repl_dboff += nwritten;
    if (repl_dboff == repl_dbsize){
        watcher.stop();
        delete &watcher;
        repl_db_f->close();
        repl_db_f = nullptr;
        repl_state = replState::CONNECTED;

        repl_tio = std::make_shared<ev::timer>();
        repl_tio ->set<narutoClient, &narutoClient::onSendIncrToSlave>(this);
        repl_tio->set(ev::get_default_loop());
        repl_tio->start(FLAGS_repl_aof_interval,FLAGS_repl_aof_interval);
        replica->addSlave(this);
        LOG(INFO) << "Synchronization db with slave succeeded";
    }
}

void naruto::narutoClient::onSendIncrToSlave(ev::timer& watcher, int event) {
    if (repl_aof_file_name.empty()) return;
    LOG(INFO) << "onSendIncrToSlave----->>2-->>"
            << " repl_aof_file_name " << repl_aof_file_name
            << " repl_aof_off " << repl_aof_off;

    std::string aofname = FLAGS_repl_dir + "/" + repl_aof_file_name;
    std::ifstream in(aofname, std::ios::in|std::ios::binary);
    if (!in.is_open()) return;
    if (in.peek() == EOF){ // 文件为空
        if (utils::File::hasNextAof(FLAGS_repl_dir, repl_aof_file_name)){
            repl_aof_off = 0;
        }
        in.close();
        return;
    }

    in.seekg(repl_aof_off, std::ios::beg);
    in.peek();
    int64_t readed = 0, pack_nums = 0;
    client::command_aof multi;
    char temp[4096];
    utils::Bytes buffer;

    while (!in.eof()){
        in.read(temp, sizeof(temp));
        buffer.putBytes((uint8_t*)temp, in.gcount());
        readed += (int64_t)in.gcount();

        while (buffer.bytesRemaining() > PACK_SIZE_LEN){
            uint32_t pack_size = buffer.getInt();
            if (buffer.bytesRemaining() < pack_size - sizeof(pack_size)){
                break; // 读取不够一个包
            }
            uint16_t fg = buffer.getShort();
            uint16_t type = buffer.getShort();

            unsigned body_size = pack_size - PACK_HEAD_LEN;
            char msg[body_size + 1];
            buffer.getBytes((uint8_t*)msg, body_size);
            msg[body_size] = '\0';
            multi.mutable_types()->Add(type);
            multi.mutable_commands()->Add(msg);
            pack_nums++;
        }
        if (readed >= TRANSFER_BUF_LEN) break;
    }

    if (in.eof() && utils::File::hasNextAof(FLAGS_repl_dir,repl_aof_file_name)){
        repl_aof_off = 0;
    }else{
        repl_aof_off +=  readed;
    }

    in.close();
    if (buffer.size() == 0) return;

    multi.set_aof_name(repl_aof_file_name);
    multi.set_aof_off(repl_aof_off);
    sendMsg(multi, client::MULTI);
    LOG(INFO) << "onSendIncrToSlave----->>10-->>send size=" << readed << " pack_nums=" << pack_nums;
}

void naruto::narutoClient::close() const {
    LOG(INFO) << "client close " << remoteAddr()  <<  " errmsg " << connect->errmsg();
    // 停止 io 事件
    if (cw_rio) cw_rio->stop();
    if (repl_rio) repl_rio->stop();
    if (repl_tio) repl_tio->stop();
    if (connect) connect->close();
    if (repl_db_f) repl_db_f->close();
}

void naruto::narutoClient::onWrite(ev::io &watcher, int events) {
    if (wbuf.size() == 0) return;
    try {
        connect->send(wbuf);
        wbuf.clear();
    } catch (std::exception& e) {
        close();
        LOG(ERROR) << "Connect to " << remoteAddr() << " fail " << e.what();
    }
    watcher.stop();
}

void naruto::narutoClient::sendMsg(const ::google::protobuf::Message & msg, uint16_t type) {
    wio.set<narutoClient, &narutoClient::onWrite>(this);
    if (worker_id == worker_num){ // repl or cluster
        wio.set(ev::get_default_loop());
    }else{
        wio.set(workers[worker_id].loop);
    }
    wio.start(connect->fd(), ev::WRITE);
    utils::Pack::serialize(msg, type, wbuf);
}

uint16_t naruto::narutoClient::sendMsg(const ::google::protobuf::Message &question, uint16_t type,
                                       ::google::protobuf::Message &answer) const {
    write(question, type);
    return read(answer);
}

uint64_t naruto::narutoClient::recvMsg(::google::protobuf::Message &msg) {
    connect->recv(rbuf);
    return rbuf.getShort(PACK_HEAD_TYPE_INDEX);
}

void naruto::narutoClient::write(const ::google::protobuf::Message& msg, uint16_t type) const {
    utils::Bytes pack;
    pack.putMessage(msg, type);
    connect->send(pack);
}

uint64_t naruto::narutoClient::read(::google::protobuf::Message& cmd) const {
    utils::Bytes pack;
    connect->recv(pack);
    return utils::Pack::deSerialize(pack, cmd);
}

std::string naruto::narutoClient::remoteAddr() const { return remote_addr; }
