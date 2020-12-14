//
// Created by kwins on 2020/12/8.
//

#include "cluster_link.h"
#include "connect_worker.h"

naruto::cluster::clusterLink::clusterLink(naruto::connection::ConnectOptions opts):link::clientLink(opts, worker_num) {}

naruto::cluster::clusterLink::clusterLink(int sd) :link::clientLink(worker_num,sd) {}

void naruto::cluster::clusterLink::regReadEvent() {
    c_rio = std::make_shared<ev::io>();
    c_rio->set<clusterLink, &clusterLink::onRead>(this);
    c_rio->set(ev::get_default_loop());
    c_rio->start(connect->fd(), ev::READ);
}

void naruto::cluster::clusterLink::onRead(ev::io &watcher, int events) {
    clientLink::onRead(watcher, events);
}

void naruto::cluster::clusterLink::onWrite(ev::io &watcher, int events) {
    clientLink::onWrite(watcher, events);
}

void naruto::cluster::clusterLink::close() const {
    clientLink::close();
}
