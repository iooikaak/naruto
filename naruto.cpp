//
// Created by 王振奎 on 2020/8/14.
//
#include "naruto.h"

#include <fstream>
#include "client.h"
#include "parameter/parameter.h"
#include "connect_worker.h"
#include "replication.h"

namespace naruto{

void Naruto::initializer() {
    // 统计信息初始化
    _tcp_backlog = FLAGS_tcp_backlog;
    _port = FLAGS_port;
    _fd = -1;
    _stat_start_time = 0;
    _stat_num_commands = 0;
    _stat_num_connections = 0;
    _stat_expire_keys = 0;
    _stat_keyspace_hits = 0;
    _stat_keyspace_miss = 0;
    _stat_peak_memory = 0;
    _stat_rejected_conn = 0;
    _stat_sync_full = 0;
    _stat_sync_partial_ok = 0;
    _stat_sync_partial_err = 0;
    _connect_nums = 0;
    _cluster_enable = false;
    // RDB / AOF
    _loading = false;
    _loading_total_bytes = 0;
    _loading_loaded_bytes = 0;
    _loading_start_time = 0;
    _loading_process_events_interval_bytes = 0;
    _clients_paused = false;
    _clients_pause_end_time = 0;
    cluster_ = std::make_shared<Cluster>(FLAGS_port, FLAGS_tcp_backlog);
}

Naruto::~Naruto() { delete [] workers; }

void Naruto::start() {
    initializer();
    _init_workers();
    _init_signal();
    _init_cron();
    _listen();
}

// onAccept 客户端连接
void Naruto::onAccept(ev::io& watcher, int events) {
    LOG(INFO) << "onAccept...";

    struct sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    int client_sd;

    if ((client_sd = accept(watcher.fd, (sockaddr*)&client_addr, &client_len)) < 0){
        LOG(ERROR) << "onAccept:" << strerror(errno);
        return;
    }

    // 创建新的client连接
    auto client = new narutoClient();
    client->connect = std::make_shared<connection::Connect>(client_sd);
    char str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, str, sizeof(str));
    client->ip = str;
    client->port = std::to_string(client_addr.sin_port);

    // 唤醒worker
    LOG(INFO) << "onAccept OK! dispatch connect, client_sd=" << client->connect->fd() << " watcher.fd=" << watcher.fd
        << " addr:" << client->ip << ":" << client->port;

    int index = _connect_nums % worker_num;
    client->worker_id = index; // record work id

    workers[index].conns.push_back(client);
    workers[index].async_watcher.send();

    _connect_nums++;
}

void Naruto::onCron(ev::timer& watcher, int event) {
}

void Naruto::onSignal(ev::sig& signal, int) {
    auto s = static_cast<Naruto*>(signal.data);
    // 关闭socket
    close(s->_fd);
    signal.stop();
    LOG(INFO) << "onSignal start...1";
    // 关闭工作线程
    exit_success_workers = 0;
    for (int i = 0; i < worker_num; ++i)  workers[i].stop();
    LOG(INFO) << "onSignal start...2";
    // 关闭cluster工作线程
    s->cluster_->stop();
    replica->stop();

    std::unique_lock<std::mutex> lck(mux);
    while (exit_success_workers < worker_num){
        LOG(INFO) << "onSignal..." << exit_success_workers;
        cond.wait(lck);
    }

    // 停止主线程 ev loop
    s->accept_watcher_.stop();
    s->timer_watcher_.stop();
    s->_loop.break_loop(ev::ALL);
    LOG(INFO) << "onSignal end...";
}

void Naruto::_init_workers() {
    srand(time(nullptr)^getpid());
    LOG(INFO) << "Init workers worker num is:" << worker_num;
    replica->initializer();

    for (int i = 0; i < worker_num; ++i) {
        workers[i].async_watcher.set<&ConnectWorker::onAsync>(&workers[i]);
        workers[i].async_watcher.set(workers[i].loop);
        workers[i].async_watcher.start();

        workers[i].stop_async_watcher.set<&ConnectWorker::onStopAsync>(&workers[i]);
        workers[i].stop_async_watcher.set(workers[i].loop);
        workers[i].stop_async_watcher.start();
    }

    // run workers in thread
    for (int j = 0; j < worker_num; ++j) {
        auto worker = &workers[j];
        std::thread([worker,j](){
            worker->run(j);
        }).detach();
    }

    if (_cluster_enable) _init_cluster();
    // 等待 worker 线程初始化完毕
    std::unique_lock<std::mutex> lck(mux);
    while (init_success_workers < worker_num){
        cond.wait(lck);
    }
}

void Naruto::_listen() {
    if ((_fd = naruto::utils::Net::listen(_port, _tcp_backlog)) == -1){
        return;
    }
    accept_watcher_.set(_loop);
    accept_watcher_.set<Naruto, &Naruto::onAccept>(this);
    accept_watcher_.start(_fd, ev::READ);
    LOG(INFO) << "Naruto listen in " << _port;
    _loop.loop(0);
    LOG(INFO) << "Naruto stop listen.";
}

void Naruto::_init_signal() {
    sigint_.set<&Naruto::onSignal>(this);
    sigint_.set(_loop);
    sigint_.start(SIGINT);
    sigterm_.set<&Naruto::onSignal>(this);
    sigterm_.set(_loop);
    sigterm_.start(SIGTERM);
    sigkill_.set<&Naruto::onSignal>(this);
    sigkill_.set(_loop);
    sigkill_.start(SIGKILL);
}

void Naruto::_init_cluster() {
    LOG(INFO) << "init cluster run...";
    std::unique_lock<std::mutex> lck(mux);
    init_success_workers++;
    cond.notify_one();
    cluster_->run();
    lck.unlock();
}

void Naruto::_init_cron() {
    timer_watcher_.set<Naruto, &Naruto::onCron>(this);
    timer_watcher_.set(_loop);
    timer_watcher_.start(FLAGS_cron_interval, FLAGS_cron_interval);
}

Naruto* server = new Naruto();

}
