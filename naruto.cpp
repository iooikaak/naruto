//
// Created by 王振奎 on 2020/8/14.
//
#include "naruto.h"

#include <fstream>
#include "link/client_link.h"
#include "replication/replica_link.h"
#include "parameter/parameter.h"

namespace naruto{

void Naruto::initializer() {
    // 统计信息初始化
    tcp_backlog_ = FLAGS_tcp_backlog;
    fd_ = -1;
    rc_fd_ = -1;
    stat_start_time_ = 0;
    stat_num_commands_ = 0;
    stat_num_connections_ = 0;
    stat_expire_keys_ = 0;
    stat_keyspace_hits_ = 0;
    stat_keyspace_miss_ = 0;
    stat_peak_memory_ = 0;
    connect_nums_ = 0;
    cluster_enable_ = false;
    clients_paused_ = false;
    clients_pause_end_time_ = 0;
    cluster_ = std::make_shared<Cluster>(FLAGS_rc_port, FLAGS_tcp_backlog);
}

Naruto::~Naruto() { delete [] workers; }

void Naruto::start() {
    this->initializer();
    replica::replptr->initializer();
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
    int index = connect_nums_ % worker_num;
    auto client = new link::clientLink(index,client_sd);
    // 唤醒worker
    LOG(INFO) << "onAccept OK! dispatch connect, client_sd=" << client->connect->fd() << " watcher.fd=" << watcher.fd
        << " addr:" << client->remote_addr;
    workers[index].conns.push_back(client);
    workers[index].async.send();
    connect_nums_++;
}

void Naruto::onAcceptRc(ev::io &watcher, int event) {
    LOG(INFO) << "onAcceptRc...";

    struct sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    int client_sd;

    if ((client_sd = accept(watcher.fd, (sockaddr*)&client_addr, &client_len)) < 0){
        LOG(ERROR) << "onAcceptRc:" << strerror(errno);
        return;
    }

    // 创建新的client连接
    auto client = new replica::replicaLink(client_sd);
    // 唤醒worker
    LOG(INFO) << "onAcceptRc OK! dispatch connect, client_sd=" << client->connect->fd() << " watcher.fd=" << watcher.fd
              << " addr:" << client->connect->remoteAddr();

    client->c_rio = std::make_shared<ev::io>();
    client->c_rio->set<replica::replicaLink, &replica::replicaLink::onRead>(client);
    client->c_rio->set(ev::get_default_loop());
    client->c_rio->start(client->connect->fd(), ev::READ);
    connect_nums_++;
}

void Naruto::onCron(ev::timer& watcher, int event) {
}

void Naruto::onSignal(ev::sig& signal, int) {
    auto s = static_cast<Naruto*>(signal.data);
    // 关闭socket
    close(s->fd_);
    close(s->rc_fd_);
    signal.stop();
    LOG(INFO) << "onSignal start...1";
    // 关闭工作线程
    exit_success_workers = 0;
    for (int i = 0; i < worker_num; ++i)  workers[i].stop();
    LOG(INFO) << "onSignal start...2";
    // 关闭cluster工作线程
    s->cluster_->stop();
    replica::replptr->stop();

    std::unique_lock<std::mutex> lck(mux);
    while (exit_success_workers < worker_num){
        LOG(INFO) << "onSignal..." << exit_success_workers;
        cond.wait(lck);
    }

    // 停止主线程 ev loop
    s->ct_rio_.stop();
    s->rc_rio_.stop();
    s->timer_watcher_.stop();
    ev::get_default_loop().break_loop(ev::ALL);
    LOG(INFO) << "onSignal end...";
}

void Naruto::_init_workers() {
    srand(time(nullptr)^getpid());
    LOG(INFO) << "Naruto worker num is:" << worker_num;
    for (int i = 0; i < worker_num; ++i) {
        workers[i].async.set<&ConnectWorker::onAsync>(&workers[i]);
        workers[i].async.set(workers[i].loop);
        workers[i].async.start();

        workers[i].stop_async.set<&ConnectWorker::onStopAsync>(&workers[i]);
        workers[i].stop_async.set(workers[i].loop);
        workers[i].stop_async.start();
    }

    // run workers in thread
    for (int j = 0; j < worker_num; ++j) {
        auto worker = &workers[j];
        std::thread([worker,j](){
            worker->run(j);
        }).detach();
    }

    if (cluster_enable_) _init_cluster();
    // 等待 worker 线程初始化完毕
    std::unique_lock<std::mutex> lck(mux);
    while (init_success_workers < worker_num){
        cond.wait(lck);
    }
}

void Naruto::_listen() {
    if ((fd_ = naruto::utils::Net::listen(FLAGS_port, tcp_backlog_)) == -1) return;
    if ((rc_fd_ = naruto::utils::Net::listen(FLAGS_rc_port, tcp_backlog_)) == -1) return;

    ct_rio_.set(ev::get_default_loop());
    ct_rio_.set<Naruto, &Naruto::onAccept>(this);
    ct_rio_.start(fd_, ev::READ);

    rc_rio_.set(ev::get_default_loop());
    rc_rio_.set<Naruto, &Naruto::onAcceptRc>(this);
    rc_rio_.start(rc_fd_, ev::READ);
    LOG(INFO) << "Naruto listen client port in " << FLAGS_port << " peers port in " << FLAGS_rc_port << ".";
    ev::get_default_loop().loop(0);
    LOG(INFO) << "Naruto stop listen.";
}

void Naruto::_init_signal() {
    sigint_.set<&Naruto::onSignal>(this);
    sigint_.set(ev::get_default_loop());
    sigint_.start(SIGINT);
    sigterm_.set<&Naruto::onSignal>(this);
    sigterm_.set(ev::get_default_loop());
    sigterm_.start(SIGTERM);
    sigkill_.set<&Naruto::onSignal>(this);
    sigkill_.set(ev::get_default_loop());
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
    timer_watcher_.set(ev::get_default_loop());
    timer_watcher_.start(FLAGS_cron_interval, FLAGS_cron_interval);
}

Naruto* server = new Naruto();

}
