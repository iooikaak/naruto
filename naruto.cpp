//
// Created by 王振奎 on 2020/8/14.
//

#include "naruto.h"
#include "parameter/parameter.h"

namespace naruto{

Naruto::Naruto() : _loop(), _cluster(FLAGS_port, FLAGS_tcp_backlog){
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
    _cron_interval = 1.0;
    _hz = 0;
    _cron_loops = 0;
    _bucket_num = FLAGS_bucket_num;
    repl = nullptr;
}

Naruto::~Naruto() { delete [] workers; }

void Naruto::run() {
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

    int index = _connect_nums % workder_num;
    client->worker_id = index; // record work id

    workers[index].conns.push_back(client);
    workers[index].async_watcher.send();

    _connect_nums++;
}

void Naruto::onCron(ev::timer& watcher, int event) {
//    LOG(INFO) << "onCron....";
}

void Naruto::onSignal(ev::sig& signal, int) {
    auto s = static_cast<Naruto*>(signal.data);
    // 关闭socket
    close(s->_fd);
    signal.stop();
    LOG(INFO) << "onSignal start...1";
    // 关闭工作线程
    exit_success_workers = 0;
    for (int i = 0; i < workder_num; ++i)  workers[i].stop();
    LOG(INFO) << "onSignal start...2";
    // 关闭cluster工作线程
    s->_cluster.stop();

    std::unique_lock<std::mutex> lck(mux);
    while (exit_success_workers < workder_num){
        LOG(INFO) << "onSignal..." << exit_success_workers;
        cond.wait(lck);
    }

    // 停止主线程 ev loop
    s->_accept_watcher.stop();
    s->_timer_watcher.stop();
    s->_loop.break_loop(ev::ALL);
    LOG(INFO) << "onSignal end...";
}

void Naruto::_init_workers() {
    LOG(INFO) << "_init_workers,workder_num=" << workder_num;
    auto cmds = std::make_shared<command::Commands>();
    auto bts = std::make_shared<database::Buckets>();
    for (int i = 0; i < workder_num; ++i) {
        workers[i].commands = cmds;
        workers[i].buckets = bts;
        workers[i].server = this;
        workers[i].async_watcher.set<&ConnectWorker::onAsync>(&workers[i]);
        workers[i].async_watcher.set(workers[i].loop);
        workers[i].async_watcher.start();

        workers[i].stop_async_watcher.set<&ConnectWorker::onStopAsync>(&workers[i]);
        workers[i].stop_async_watcher.set(workers[i].loop);
        workers[i].stop_async_watcher.start();
    }

    LOG(INFO) << "_init_workers...2";

    // run workers in thread
    for (int j = 0; j < workder_num; ++j) {
        auto worker = &workers[j];
        std::thread([worker,j](){
            worker->run(j);
        }).detach();
    }

    LOG(INFO) << "_init_workers...3";
//    std::this_thread::sleep_for(std::chrono::seconds(5));
    LOG(INFO) << "_init_workers...4, init_success_workers:" << init_success_workers;
    if (_cluster_enable) _init_cluster();
    LOG(INFO) << "_init_workers...5";
    // 等待 worker 线程初始化完毕
    std::unique_lock<std::mutex> lck(mux);
    while (init_success_workers < workder_num){
        cond.wait(lck);
    }
    LOG(INFO) << "_init_workers end..." << init_success_workers;
}

void Naruto::_listen() {
    if ((_fd = naruto::utils::Net::listen(_port, _tcp_backlog)) == -1){
        return;
    }
    _accept_watcher.set(_loop);
    _accept_watcher.set<Naruto, &Naruto::onAccept>(this);
    _accept_watcher.start(_fd, ev::READ);
    LOG(INFO) << "Naruto listen in " << _port;
    _loop.loop(0);
    LOG(INFO) << "Naruto stop listen.";
}

void Naruto::_init_signal() {
    _sigint.set<&Naruto::onSignal>(this);
    _sigint.set(_loop);
    _sigint.start(SIGINT);
    _sigterm.set<&Naruto::onSignal>(this);
    _sigterm.set(_loop);
    _sigterm.start(SIGTERM);
    _sigkill.set<&Naruto::onSignal>(this);
    _sigkill.set(_loop);
    _sigkill.start(SIGKILL);
}

void Naruto::_init_cluster() {
    LOG(INFO) << "init cluster run...";
    std::unique_lock<std::mutex> lck(mux);
    init_success_workers++;
    cond.notify_one();
    _cluster.run();
    lck.unlock();
}

void Naruto::_init_cron() {
    _timer_watcher.set<&Naruto::onCron>(this);
    _timer_watcher.set(_loop);
    _timer_watcher.start(_cron_interval, _cron_interval);
}

}
