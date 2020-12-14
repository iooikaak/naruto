//
// Created by kwins on 2020/12/8.
//

#include "cluster.h"

#include <ev++.h>
#include "parameter/parameter_cluster.h"
using namespace std::chrono;

void naruto::cluster::Cluster::initializer() {

}

void naruto::cluster::Cluster::onClusterCron(ev::timer &watcher, int event) {
    iteration++;
    // 1.向集群中的所有断线或者未连接节点发送消息
    view.checkLink();
    // 2.clusterCron() 每执行 10 次（至少间隔一秒钟）随机节点发送 gossip 信息
    if (!(iteration % 10)) view.gossip();
    // 3.遍历所有节点，检查是否需要将某个节点标记为下线
    auto update_state = view.checkOffline();
    // 4.如果从节点没有在复制主节点，那么对从节点进行设置
    view.checkReplica();
    // 5.更新集群状态
    if (update_state && view.state == clusterState::FAIL){
        view.updateView();
    }
}

std::shared_ptr<naruto::cluster::Cluster> naruto::cluster::clsptr = std::make_shared<naruto::cluster::Cluster>();

