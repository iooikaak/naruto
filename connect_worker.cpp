//
// Created by 王振奎 on 2020/8/19.
//



#include "connect_worker.h"

#include "replication.h"

namespace naruto{

ConnectWorker::ConnectWorker(): loop(), async_watcher(),conns(0),tid(0){}

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

void ConnectWorker::clientFree(narutoClient *nc) {
    if (nc == nullptr) return;
    conn_nums--;
    LOG(INFO) << "connect worker: peer " << nc->remoteAddr()  <<  " " << nc->connect->errmsg();
    // 停止 io 事件
    nc->cw_io->stop();
    if (nc->connect)
        nc->connect->close();

    switch (nc->flag) {
        case narutoClient::flags::MASTER:
            replica->cacheMaster();
            break;
        case narutoClient::flags::SLAVE:
            replica->statSlave();
            break;
        default:
            break;
    }

    delete nc;
}

void ConnectWorker::clientFree(std::shared_ptr<narutoClient>& nc) { clientFree(nc.get()); }

void ConnectWorker::onAsync(ev::async& watcher, int events) {
    auto worker = static_cast<ConnectWorker*>(watcher.data);
    while (!worker->conns.empty()){
        // 这里的连接包含了slave的连接
        narutoClient* nc = worker->conns.front();
        if (!nc) return;

        worker->conns.pop_front();

        nc->cw_io = std::make_shared<ev::io>();
        nc->cw_io->set<naruto::narutoClient, &naruto::narutoClient::onRead>(nc);
        nc->cw_io->set(worker->loop);
        nc->cw_io->start(nc->connect->fd(), ev::READ);
        worker->conn_nums++;
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