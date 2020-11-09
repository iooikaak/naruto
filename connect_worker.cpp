//
// Created by 王振奎 on 2020/8/19.
//



#include "connect_worker.h"

namespace naruto{

ConnectWorker::ConnectWorker(): loop(), async_watcher(),conns(0),tid(0){}

ConnectWorker::~ConnectWorker() {
    LOG(INFO) << "~ConnectWorker stop, close conns..." << tid;
    while (!conns.empty()){
        auto c = conns.front();
        conns.pop_front();
        c->connect->close();
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

    LOG(INFO) << "connect worker run tid: " << tid << " init_success_workers: " << init_success_workers;
    loop.loop(0);
    LOG(INFO) << "connect worker run tid: " << tid << " break.";
    std::unique_lock<std::mutex> lck1(mux);
    exit_success_workers++;
    cond.notify_one();
    LOG(INFO) << "connect worker run end......" << tid;
}

void ConnectWorker::freeClient(ev::io &watcher, narutoClient *client) {
    LOG(INFO) << "connect worker: peer " << client->remoteAddr()  <<  " " << client->connect->errmsg();
    // 停止 io 事件
    watcher.stop();
    delete &watcher;
    client->connect->close();
    if (client->flags & CLIENT_FLAG_MASTER){

    }

    if (client->flags & CLIENT_FLAG_SLAVE){

    }
    delete client;
}

void ConnectWorker::onAsync(ev::async& watcher, int events) {
    auto worker = static_cast<ConnectWorker*>(watcher.data);
    while (!worker->conns.empty()){
        // 这里的连接包含了slave的连接
        narutoClient* client = worker->conns.front();
        if (!client) return;

        worker->conns.pop_front();

        auto* r = new ev::io;
        r->set<naruto::narutoClient, &naruto::narutoClient::onReadEvent>(client);
        r->set(worker->loop);
        r->start(client->connect->fd(), ev::READ);
    }
}

void ConnectWorker::onStopAsync(ev::async &watcher, int events) {
    auto cw = static_cast<ConnectWorker*>(watcher.data);
    watcher.stop();
    cw->loop.break_loop(ev::ALL);
}

void ConnectWorker::stop() { stop_async_watcher.send(); }


int worker_num = (int)(sysconf(_SC_NPROCESSORS_CONF) * 2) - 1;;
ConnectWorker* workers = new ConnectWorker[worker_num];

}