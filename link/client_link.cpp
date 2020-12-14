//
// Created by kwins on 2020/12/8.
//

#include "client_link.h"

#include "command/commands.h"
#include "protocol/command_types.pb.h"
#include "utils/errors.h"
#include "connect_worker.h"
#include "utils/pack.h"

naruto::link::clientLink::clientLink(naruto::connection::ConnectOptions opts, int wid) {
    worker_id = wid;
    connect = std::make_shared<connection::Connect>(opts);
    if (connect->connect() != CONNECT_RT_OK){
        LOG(ERROR) << "Fail connecting to MASTER "
                   << opts.host<< ":"
                   << opts.port << " errmsg:"
                   << connect->errmsg();
        return;
    }
    remote_addr = connect->remoteAddr();
    ctime = std::chrono::system_clock::now();
    flag = 0;
}

naruto::link::clientLink::clientLink(int wid, int sd) {
    worker_id = wid;
    connect = std::make_shared<connection::Connect>(sd);
    remote_addr = connect->remoteAddr();
    ctime = std::chrono::system_clock::now();
    flag = 0;
}

void naruto::link::clientLink::onRead(ev::io &watcher, int events) {
    lastinteraction = std::chrono::steady_clock::now();
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
              << " type:" << cmdtype::Type_descriptor()->FindValueByNumber(type)->name();

    command::commands->fetch(type)->exec(this);
}

void naruto::link::clientLink::onWrite(ev::io &watcher, int events) {
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

void naruto::link::clientLink::sendMsg(const google::protobuf::Message &msg, uint16_t type) {
    c_wio.set<clientLink, &clientLink::onWrite>(this);
    if (worker_id == worker_num){ // repl or cluster
        c_wio.set(ev::get_default_loop());
    }else{
        c_wio.set(workers[worker_id].loop);
    }
    c_wio.start(connect->fd(), ev::WRITE);
    utils::Pack::serialize(msg, type, wbuf);
}

uint16_t naruto::link::clientLink::sendMsg(
        const google::protobuf::Message &question,uint16_t type,
        google::protobuf::Message &answer) const {
    write(question, type);
    return read(answer);
}

uint64_t naruto::link::clientLink::recvMsg(google::protobuf::Message &msg) {
    connect->recv(rbuf);
    return rbuf.getShort(PACK_HEAD_TYPE_INDEX);
}

uint64_t naruto::link::clientLink::read(google::protobuf::Message &msg) const {
    utils::Bytes pack;
    connect->recv(pack);
    return utils::Pack::deSerialize(pack, msg);
}

void naruto::link::clientLink::write(const google::protobuf::Message &msg, uint16_t type) const {
    utils::Bytes pack;
    pack.putMessage(msg, type);
    connect->send(pack);
}

void naruto::link::clientLink::close() const {
    LOG(INFO) << "client link close " << remoteAddr()  <<  " errmsg " << connect->errmsg();
    // 停止 io 事件
    if (c_rio) c_rio->stop();
    if (connect) connect->close();
}

std::string naruto::link::clientLink::remoteAddr() const { return remote_addr; }
