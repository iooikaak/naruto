//
// Created by 王振奎 on 2020/8/19.
//


#include <glog/logging.h>
#include <sys/socket.h>

#include "utils/errors.h"
#include "connect_worker.h"
#include "protocol/message.pb.h"
#include "protocol/message_type.h"

namespace naruto{

ConnectWorker::ConnectWorker(): loop(), async_watcher(),conns(0),tid(0){}

ConnectWorker::~ConnectWorker() {
    LOG(INFO) << "~ConnectWorker stop, close conns..." << tid;
    while (!conns.empty()){
        auto c = conns.front();
        conns.pop_front();
        close(c->_fd);
        delete c;
        c = nullptr;
    }
}

void ConnectWorker::run(int id) {
    tid = id;
    LOG(INFO) << "connect worker run..." << tid;
    std::unique_lock<std::mutex> lck(mux);
    init_success_workers++;
    cond.notify_one();
    lck.unlock();

    LOG(INFO) << "connect worker run notify..." << tid << " init_success_workers:" << init_success_workers;
    loop.loop(0);

    std::unique_lock<std::mutex> lck1(mux);
    exit_success_workers++;
    cond.notify_one();
    LOG(INFO) << "connect worker run end......" << tid;
}

void ConnectWorker::onEvents(ev::io& watcher, int events) {
    if (ev::ERROR & events){
        LOG(ERROR) << "connect worker error";
        return;
    }
    LOG(INFO) << "onEvents worker:" << tid << " watcher.fd=" << watcher.fd << " read:" << (EV_READ & events) << " write:" << (EV_WRITE & events);

    auto* c = (Connect*) watcher.data;
    naruto::utils::Bytes pack;
    try {
        c->recv(pack);
    }catch (utils::Error& e){
        _close_connect(watcher, c);
        return;
    }

    uint32_t pack_size = pack.getInt();
    uint8_t version = pack.get();
    uint8_t flag = pack.get();
    uint16_t type = pack.getShort();
    LOG(INFO) <<" recv:" << c->_remote_addr() << " pack_size:" << pack_size << " version:" << (unsigned)version << " flag:" << (unsigned)flag << " type:" << (unsigned)type;
    if (type == COMMAND_HMGET){
        protocol::command_hmget cmd;
        cmd.ParseFromArray(&pack.data()[PACK_HEAD_LEN], pack_size-PACK_HEAD_LEN);
        LOG(INFO) << "cmd:" << cmd.DebugString();
    }
}

void ConnectWorker::_close_connect(ev::io& watcher, Connect* c) {
    c->closeConnect();
    watcher.stop();
    delete &watcher;
    LOG(INFO) << "connect worker: peer " << c->_remote_addr()  <<  " might closed";
    delete c;
}

void ConnectWorker::onAsync(ev::async& watcher, int events) {
    LOG(INFO) << "onAsync...";
    auto worker = static_cast<ConnectWorker*>(watcher.data);
    while (!worker->conns.empty()){
        // connect 不会在这里 delete ，会带到下一次 onEvents中
        // 处理完 READ 或 WRITE 事件后再 delete，因为在后会使用 connect 信息
        Connect* c = worker->conns.front();
        worker->conns.pop_front();
        if (c == nullptr){
            LOG(INFO) << "onAsync item nullptr...";
            return;
        }

        LOG(INFO)<< "onAsync item..." << c->_fd;
        auto* w = new ev::io;
        w->set<ConnectWorker, &ConnectWorker::onEvents>(worker);
        w->data = (void*)c;
        w->set(worker->loop);
        w->start(c->_fd, ev::READ);
    }
}

void ConnectWorker::onStopAsync(ev::async &watcher, int events) {
    LOG(INFO) << "onStopAsync ...";
    auto cw = static_cast<ConnectWorker*>(watcher.data);
    watcher.stop();
    cw->loop.break_loop(ev::ALL);
}

void ConnectWorker::stop() {
    stop_async_watcher.send();
}

}