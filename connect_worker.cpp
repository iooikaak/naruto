//
// Created by 王振奎 on 2020/8/19.
//



#include "connect_worker.h"

#include "replication.h"

namespace naruto{

ConnectWorker::ConnectWorker(): loop(), async(), conns(0), tid(0){}

ConnectWorker::~ConnectWorker() {
//    LOG(INFO) << "~ConnectWorker stop, close conns..." << tid;
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
//    LOG(INFO) << "connect worker run..." << tid;
    std::unique_lock<std::mutex> lck(mux);
    init_success_workers++;
    cond.notify_one();
    lck.unlock();

//    LOG(INFO) << "connect worker run tid: " << tid << " init_success_workers: " << init_success_workers;
    loop.loop(0);
//    LOG(INFO) << "connect worker run tid: " << tid << " break.";
    std::unique_lock<std::mutex> lck1(mux);
    exit_success_workers++;
    cond.notify_one();
//    LOG(INFO) << "connect worker run end......" << tid;
}

void ConnectWorker::onAsync(ev::async& watcher, int events) {
    auto worker = static_cast<ConnectWorker*>(watcher.data);
    while (!worker->conns.empty()){
        // 这里的连接包含了slave的连接
        narutoClient* nc = worker->conns.front();
        if (!nc) return;

        worker->conns.pop_front();

        nc->cw_rio = std::make_shared<ev::io>();
        nc->cw_rio->set<naruto::narutoClient, &naruto::narutoClient::onRead>(nc);
        nc->cw_rio->set(worker->loop);
        nc->cw_rio->start(nc->connect->fd(), ev::READ);
        worker->conn_nums++;
    }
}

void ConnectWorker::onStopAsync(ev::async &watcher, int events) {
    auto cw = static_cast<ConnectWorker*>(watcher.data);
    watcher.stop();
    cw->loop.break_loop(ev::ALL);
}

void ConnectWorker::stop() { stop_async.send(); }


int worker_num = (int)(sysconf(_SC_NPROCESSORS_CONF) * 2) - 1;;
ConnectWorker* workers = new ConnectWorker[worker_num];

}