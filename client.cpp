//
// Created by 王振奎 on 2020/9/27.
//

#include "client.h"

#include "global.h"
#include "naruto.h"
#include "connect_worker.h"
#include "replication.h"
#include "command/commands.h"

naruto::narutoClient::narutoClient() {
    flags = 0;
    ctime = std::chrono::steady_clock::now();
    repl_run_id = "";
    repl_state = state::NONE;
    repl_ack_time = std::chrono::steady_clock::now();
    repl_fd = -1;
    repl_db_off = 0;
    repl_db_size = 0;
    repl_off = 0;
    lastinteraction = std::chrono::steady_clock::now();
}

void naruto::narutoClient::onReadEvent(ev::io &watcher, int events) {
    if (ev::ERROR & events){
        LOG(ERROR) << "connect worker error";
        return;
    }
    LOG(INFO) << "onReadEvent------>> " << worker_id << " step 0";

    try {
        rbuf.clear();
        connect->recv(rbuf);
    }catch (utils::Error& e){
        workers[worker_id].freeClient(watcher, this);
        return;
    }

    uint32_t pack_size = rbuf.getInt();
    uint8_t version = rbuf.get();
    uint8_t flag = rbuf.get();
    uint16_t type = rbuf.getShort();

    LOG(INFO) << "onReadEvent------>> work:" << worker_id << " recv:" << remoteAddr() << " pack_size:"
              << pack_size << " version:" << (unsigned)version << " flag:"
              << (unsigned)flag << " type:" << (unsigned)type;


    command::commands->fetch(type)->exec(this);
    server->repl->backlogFeed(worker_id, rbuf);
}

void naruto::narutoClient::onWriteEvent(ev::io &watcher, int events) {
    LOG(INFO) << "onWriteEvent--->>1";
    if (wbuf.size() > 0){
        LOG(INFO) << "onWriteEvent--->>2";
        connect->send(wbuf);
        wbuf.clear();
    }else{
        LOG(INFO) << "onWriteEvent--->>3";
    }

    // 删除写事件
    watcher.stop();
}

void naruto::narutoClient::sendMsg(const ::google::protobuf::Message & msg,
        uint16_t type) {
    // 安装写事件
    wio.set<narutoClient, &narutoClient::onWriteEvent>(this);
    wio.set(workers[worker_id].loop);
    wio.start(connect->fd(), ev::WRITE);
    // 写到 client wbuf
    utils::Pack::serialize(msg, type, wbuf);
    LOG(INFO) << "sendMsg--->>" << wbuf.size();
}

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
    return utils::Pack::deSerialize(pack, cmd);
}

std::string naruto::narutoClient::remoteAddr() { return ip + ":" + port; }
