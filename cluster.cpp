//
// Created by 王振奎 on 2020/8/11.
//
#include <glog/logging.h>
#include <netinet/in.h>
#include <ev++.h>

#include "cluster.h"
#include "utils/net.h"
#include "global.h"

naruto::Cluster::Cluster(int port, int tcp_backlop){
    _port = port;
    _tcp_backlog = tcp_backlop;
    _connect_nums = 0;
}

naruto::Cluster::~Cluster(){

}

void naruto::Cluster::run(){
    LOG(INFO) << "cluster run..." << _port + CLUSTER_BASE_PORT;
    if ((_fd = naruto::utils::Net::listen(_port + CLUSTER_BASE_PORT,
            _tcp_backlog)) == -1){
        return;
    }
    LOG(INFO) << "cluster run..2";
    _cluster_ev_io_r = std::make_shared<ev::io>();
    _cluster_ev_io_r->set<Cluster, &Cluster::onAccept>(this);
    _cluster_ev_io_r->set(ev::get_default_loop());
    _cluster_ev_io_r->start(_fd, ev::READ);
    LOG(INFO) << "cluster run...3";
}

void naruto::Cluster::stop() {
    if (_cluster_ev_io_r) _cluster_ev_io_r->stop();
    if (_cluster_ev_io_w) _cluster_ev_io_w->stop();
}

void naruto::Cluster::onAccept(ev::io& watcher, int events){
    LOG(INFO) << "cluster onAccept...";

    struct sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    int client_sd;

    if ((client_sd = accept(watcher.fd, (sockaddr*)&client_addr, &client_len)) < 0){
        LOG(ERROR) << "onAccept:" << strerror(errno);
        return;
    }

    // 创建新的连接
    naruto::connection::ConnectOptions options;
    auto* c = new connection::Connect(client_sd);

    // 唤醒 worker
    LOG(INFO) << "cluster onAccept OK! dispatch connect, client_sd=" << client_sd << " watcher.fd=" << watcher.fd;

    auto* w = new ev::io;
    _cluster_ev_io_r->set<Cluster, &Cluster::onEvents>(this);
    _cluster_ev_io_r->data = (void*)c;
    _cluster_ev_io_r->set(ev::get_default_loop());
    _cluster_ev_io_r->start(c->fd(), ev::READ);

    _connect_nums++;
}


void naruto::Cluster::onEvents(ev::io& watcher, int events){

}

void naruto::Cluster::onPing(ev::timer &watcher, int events) {
    auto now = std::chrono::steady_clock::now();
    for (auto node : _nodes){
        std::string nodename = node.first;
        // 跳过当前节点以及没有地址的节点

        if (node.second->flags & (CLUSTER_NODE_MYSELF | CLUSTER_NODE_NOADDR)) continue;

        auto time_span = std::chrono::duration_cast<std::chrono::duration<double >>(now - node.second->ctime);
        if (node.second->inHandshake() && time_span.count() * 1000 > _cluster_node_timeout.count()){
            _remove_node(nodename);
            continue;
        }

        // 发送 Ping 消息
        if (node.second->link == nullptr){
            // 为未创建连接的节点创建连接
            naruto::connection::ConnectOptions options;
            options.host = node.second->ip;
            options.port = this->_port + CLUSTER_BASE_PORT;
            node.second->link = new naruto::connection::Connect(options);

            if (node.second->link->connect() == CONNECT_RT_ERR){
                LOG(ERROR) << "unable to connect to cluster node ["<< node.second->ip << "]:"
                            << options.port
                            << " -> " << node.second->link->errmsg();
                continue;
            }

            // 加入 epoll 事件中
            auto* w = new ev::io;
            w->set<Cluster, &Cluster::onEvents>(this);
            w->data = (void*)node.second->link;
            w->set(ev::get_default_loop());
            w->start(node.second->link->fd(), ev::READ);

            // 发送 Ping 消息

        }
    }
}

void naruto::Cluster::_remove_node(const std::string& name) {
    auto n = _nodes.find(name);
    if (n != _nodes.end()){
        // 移除从节点
        if (n->second->slaveof != nullptr){
            _remove_slave_of_master(n->second->slaveof, n->second);
        }
        // 释放连接

    }
    _nodes.erase(name);
}

void naruto::Cluster::_remove_slave_of_master(
        std::shared_ptr<clusterNode> master, std::shared_ptr<clusterNode> slave) {
    master->slaves.remove(slave);
}


void naruto::Cluster::_send_ping(std::shared_ptr<clusterNode> node, int type) {
    int freshnodes = _nodes.size() - 2;
    if (type == CLUSTER_MESSAGE_TYPE_PING){
        node->ping_sent = std::chrono::steady_clock::now().time_since_epoch().count();
    }

    std::shared_ptr<clusterNode> master;
    master = (node->isSlave() && node->slaveof != nullptr) ? node->slaveof : _myself;

    // build message
    cluster::command_gossip ping;
    auto header = ping.mutable_header();
    header->set_current_epoch(_currentEpoch);
    header->set_config_epoch(master->config_epoch);


}
