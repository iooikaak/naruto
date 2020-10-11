//
// Created by 王振奎 on 2020/9/27.
//

#include "client.h"
#include "replication.h"
#include "global.h"
#include "connect_worker.h"

naruto::narutoClient::narutoClient() {
    flags = 0;
    ctime = std::chrono::steady_clock::now();
    repl_run_id = "";
    repl_state = REPL_STATE_NONE;
    repl_ack_time = std::chrono::steady_clock::now();
    repl_fd = -1;
    repl_db_off = 0;
    repl_db_size = 0;
    repl_off = 0;
    lastinteraction = std::chrono::steady_clock::now();
    wbuf.resize(CLIENT_BUF_SIZE);
    rbuf.resize(CLIENT_BUF_SIZE);
}

void naruto::narutoClient::onRead(ev::io &watcher, int events) {
    if (ev::ERROR & events){
        LOG(ERROR) << "connect worker error";
        return;
    }
    LOG(INFO) << "------>>onRead worker " << worker_id << " step 0";

    try {
        rbuf.clear();
        connect->recv(rbuf);
    }catch (utils::Error& e){
        workers[worker_id].freeClient(watcher, this);
        return;
    }

    LOG(INFO) << "onRead worker " << worker_id << " step 1";
    uint32_t pack_size = rbuf.getInt();
    uint8_t version = rbuf.get();
    uint8_t flag = rbuf.get();
    uint16_t type = rbuf.getShort();
    LOG(INFO) <<" recv:" << remoteAddr() << " pack_size:"
              << pack_size << " version:" << (unsigned)version << " flag:"
              << (unsigned)flag << " type:" << (unsigned)type;

    LOG(INFO) << "worker tid:" << workers[worker_id].tid;
    wbuf.clear();
    workers[worker_id].commands->fetch(type)->call(workers[worker_id].buckets, rbuf, wbuf);

    LOG(INFO) << "onRead worker " << worker_id << " step 2";
    connect->send(wbuf);
    wbuf.clear();
    rbuf.clear();
    // feed slaves
}

void naruto::narutoClient::onWrite(ev::io &watcher, int events) {
    LOG(INFO) << "onWrite...";
    if (wbuf.size() > 0){
        LOG(INFO) << "onWrite...1";
        connect->send(wbuf);
        wbuf.clear();
    }else{
        LOG(INFO) << "onWrite...2";
    }
}

void naruto::narutoClient::sendMsg(const ::google::protobuf::Message & msg,
        uint16_t type) { wbuf.putMessage(msg, type); }

uint64_t naruto::narutoClient::recvMsg(::google::protobuf::Message &msg) {
    connect->recv(rbuf);
    return rbuf.getShort(PACK_HEAD_TYPE_INDEX);
}

void naruto::narutoClient::write(const ::google::protobuf::Message& msg, uint16_t type) {
    utils::Bytes pack;
    pack.putMessage(msg, type);
    connect->send(pack);
}

uint64_t naruto::narutoClient::read(::google::protobuf::Message& cmd) {
    utils::Bytes pack;
    connect->recv(pack);
    auto len = pack.getInt();
    auto version = pack.get();
    auto flag = pack.get();
    auto type = pack.getShort();

    cmd.ParseFromArray(&pack.data()[PACK_HEAD_LEN], len-PACK_HEAD_LEN);
    return (uint16_t)type;
}

std::string naruto::narutoClient::remoteAddr() { return ip + ":" + port; }
