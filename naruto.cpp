//
// Created by 王振奎 on 2020/8/14.
//
#include <deque>
#include <glog/logging.h>
#include <cstring>
#include <arpa/inet.h>
#include <fcntl.h>

#include "naruto.h"
#include "s_connect.h"
#include "utils/net.h"

namespace naruto{

Naruto::Naruto(int port, int tcp_backlog) : _port(port), _tcp_backlog(tcp_backlog),
                                            _loop(), _cluster(port, tcp_backlog){
    // 统计信息初始化
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
    _cluster_enable = true;
    // RDB / AOF
    _loading = false;
    _loading_total_bytes = 0;
    _loading_loaded_bytes = 0;
    _loading_start_time = 0;
    _loading_process_events_interval_bytes = 0;
    _clients_paused = false;
    _clients_pause_end_time = 0;

    _hz = 0;
    _cron_loops = 0;
}

Naruto::~Naruto() { delete [] _workers; }

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

    // 创建新的连接
    naruto::net::ConnectOptions options;
    auto* c = new Connect(options, watcher.fd);
    c->_fd = (client_sd);
    c->_addr = (struct sockaddr*)malloc(sizeof(struct sockaddr));
    auto* addr = (struct sockaddr*)(&client_addr);
    memcpy(c->_addr, addr, sizeof(struct sockaddr));

    // 唤醒worker
    LOG(INFO) << "onAccept OK! dispatch connect, client_sd=" << client_sd << " watcher.fd=" << watcher.fd << " addr:" << c->_remote_addr();
    int robin = _connect_nums % _worker_num;
    _workers[robin].conns.push_back(c);
    _workers[robin].async_watcher.send();
    _connect_nums++;
}

void Naruto::onSignal(ev::sig& signal, int) {
    auto s = static_cast<Naruto*>(signal.data);
    // 关闭socket
    close(s->_fd);
    signal.stop();
    // 关闭工作线程
    for (int i = 0; i < s->_worker_num; ++i)  s->_workers[i].stop();
    // 关闭cluster工作线程
    s->_cluster.stop();

    LOG(INFO) << "onSignal...";
    std::unique_lock<std::mutex> lck(mux);
    while (exit_success_workers < s->_worker_num){
        LOG(INFO) << "onSignal..." << exit_success_workers;
        cond.wait(lck);
    }
    // 停止主线程 ev loop
    s->_accept_watcher.stop();
    s->_loop.break_loop(ev::ALL);
    LOG(INFO) << "onSignal...2";
}

void Naruto::run() {
    LOG(INFO) << "naruto run:" << _port;
    _init_workers();
    _init_signal();
    _listen();
}

void Naruto::_init_workers() {
    LOG(INFO) << "_init_workers...";
    _worker_num = (int)(sysconf(_SC_NPROCESSORS_CONF) *2);
    int client_worker_num;
    if (_cluster_enable){
        client_worker_num = _worker_num-1;
    }else{
        client_worker_num = _worker_num;
    }

    _workers = new ConnectWorker[client_worker_num];

    for (int i = 0; i < client_worker_num; ++i) {
        _workers[i].async_watcher.set<&ConnectWorker::onAsync>(&_workers[i]);
        _workers[i].async_watcher.set(_workers[i].loop);
        _workers[i].async_watcher.start();

        _workers[i].stop_async_watcher.set<&ConnectWorker::onStopAsync>(&_workers[i]);
        _workers[i].stop_async_watcher.set(_workers[i].loop);
        _workers[i].stop_async_watcher.start();
    }

    // run workers in thread
    for (int j = 0; j < client_worker_num; ++j) {
        auto worker = &_workers[j];
        std::thread([worker,j](){
            worker->run(j);
        }).detach();
    }

    if (_cluster_enable) _init_cluster();

    // 等待 worker 线程初始化完毕
    std::unique_lock<std::mutex> lck(mux);
    while (init_success_workers < _worker_num){
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
    LOG(INFO) << "naruto listen loop start...";
    _loop.loop(0);
    LOG(INFO) << "_listen end...1";
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
    LOG(INFO) << "cluster run...";
    std::unique_lock<std::mutex> lck(mux);
    init_success_workers++;
    cond.notify_one();
    std::thread([this]{ _cluster.run(); });
    lck.unlock();
}

}
