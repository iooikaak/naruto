//
// Created by 王振奎 on 2020/9/27.
//

#include "client.h"

#include "global.h"
#include "connect_worker.h"
#include "replication.h"
#include "command/commands.h"
#include "protocol/client.pb.h"

naruto::narutoClient::narutoClient() {
    flag = flags::NONE;
    repl_state = replState::NONE;
    repl_run_id = "";
    repl_aof_file_name = "";
    repl_aof_off = -1;
    ctime = std::chrono::system_clock::now();
    repl_ack_time = std::chrono::steady_clock::now();
    lastinteraction = std::chrono::steady_clock::now();
}

void naruto::narutoClient::onRead(ev::io &watcher, int events) {
    lastinteraction = std::chrono::steady_clock::now();
    if (ev::ERROR & events){
        LOG(ERROR) << "connect worker error";
        return;
    }
    LOG(INFO) << "onRead------>> " << worker_id << " step 0";

    try {
        rbuf.clear();
        connect->recv(rbuf);
    }catch (utils::Error& e){
        workers[worker_id].clientFree(this);
        return;
    }

    uint32_t pack_size = rbuf.getInt();
    uint8_t version = rbuf.get();
    uint8_t flag = rbuf.get();
    uint16_t type = rbuf.getShort();

    LOG(INFO) << "onRead------>> work:" << worker_id << " recv:" << remoteAddr() << " pack_size:"
              << pack_size << " version:" << (unsigned)version << " flag:"
              << (unsigned)flag << " type:" << (unsigned)type;

    command::commands->fetch(type)->exec(this);
    switch ((client::Type)type) {
        case client::HSET:
            replica->backlogFeed(worker_id, rbuf);
            break;
        default:
            break;
    }
}

void naruto::narutoClient::onWrite(ev::io &watcher, int events) {
    watcher.stop();
    if (wbuf.size() == 0) return;
    try {
        connect->send(wbuf);
        wbuf.clear();
    } catch (std::exception& e) {
        LOG(ERROR) << "Connect to " << remoteAddr() << " fail " << e.what();
        workers[worker_id].clientFree(this);
    }
}

void naruto::narutoClient::sendMsg(const ::google::protobuf::Message & msg,
        uint16_t type) {
    // 安装写事件
    wio.set<narutoClient, &narutoClient::onWrite>(this);
    wio.set(workers[worker_id].loop);
    wio.start(connect->fd(), ev::WRITE);
    // 写到 client wbuf
    utils::Pack::serialize(msg, type, wbuf);
}

uint16_t naruto::narutoClient::sendMsg(const ::google::protobuf::Message &question, uint16_t type,
                                       ::google::protobuf::Message &answer) {
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

std::string naruto::narutoClient::remoteAddr() const { return ip + ":" + port; }
