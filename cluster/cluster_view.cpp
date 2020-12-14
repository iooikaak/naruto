//
// Created by kwins on 2020/12/8.
//

#include "cluster_view.h"
#include <random>
#include "protocol/cluster_message.pb.h"
#include "utils/basic.h"
#include "replication/replication.h"

using namespace std::chrono;
DECLARE_int32(cluster_node_timeout_sec);
DECLARE_int32(cluster_handshake_timeout_ms);
DECLARE_int32(cluster_writable_delay_ms);

void naruto::cluster::clusterView::ping(const std::shared_ptr<clusterNode>& node, cmdtype::Type type) {
    ::cluster::command_gossip msg;
    // header
    auto header = msg.mutable_header();
    header->set_current_epoch(current_epoch);
    int64_t config_epoch = 0;
    if (node->isSlave() && myself->slaveof){
        config_epoch = myself->slaveof->config_epoch;
    }else{
        config_epoch = myself->config_epoch;
    }
    header->set_config_epoch(config_epoch);
    header->set_sender_name(myself->name);
    if (myself->slaveof){
        header->set_slaveof_name(myself->slaveof->name);
    }
    for(auto v : myself->slots){
        header->add_myslots(v);
    }
    header->set_port(myself->port);
    header->set_flags(myself->flags);
    header->set_state((int64_t)state);

    // body
    int freshnodes = (int)nodes.size() -2;
    int gossipcount = 0;
    while (freshnodes > 0 && gossipcount < 3){
        auto other = randomNode();
        if (other == myself || other->isHandshake()
                || (!other->link && other->numslots == 0)){
            freshnodes--;
            continue;
        }
        bool hasin;
        for (int i = 0; i < msg.gossips_size(); ++i) {
            if (msg.gossips(i).node_name() == other->name){
                hasin = true;
            }
        }
        if (hasin) continue;
        freshnodes--;
        auto gossip = msg.add_gossips();
        gossip->set_node_name(other->name);
        gossip->set_flags(other->flags);
        gossip->set_ip(other->ip);
        gossip->set_port(other->port);
        gossip->set_ping_sent(other->ping_sent);
        gossip->set_pong_received(other->pong_received);
        gossipcount++;
    }
    node->link->sendMsg(msg, (uint16_t)type);
}

// 向一个随机节点发送 gossip 信息
void naruto::cluster::clusterView::gossip() {
    std::shared_ptr<clusterNode> min_pong_node = nullptr;
    for (int i = 0; i < 5; ++i) {
        std::shared_ptr<clusterNode> rdn = randomNode();
        if (!rdn->link || rdn->ping_sent != 0) continue;
        if (rdn->isSlave() || rdn->isHandshake()) continue;
        if (!min_pong_node || min_pong_node->pong_received > rdn->pong_received){
            min_pong_node = rdn;
        }
    }
    if (min_pong_node) {
        long long old_ping_send = min_pong_node->ping_sent;
        min_pong_node->ping_sent = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
        ping(min_pong_node, cmdtype::Type::CLUSTER_PING);
        if (old_ping_send){
            min_pong_node->ping_sent = old_ping_send;
        }
    }
}

bool naruto::cluster::clusterView::checkOffline() {
    bool update_state = 0;
    for(auto& node : nodes){
        if (node.second->isMyself() || node.second->isHandshake())
            continue;
        // TODO: Orphaned master check
        auto now_sec = duration_cast<seconds>(system_clock::now().time_since_epoch());
        if (node.second->link){
            auto node_ctime_sec = duration_cast<seconds>(node.second->link->ctime.time_since_epoch());
            if (now_sec.count() - node_ctime_sec.count() > FLAGS_cluster_node_timeout_sec &&
                node.second->ping_sent > 0 &&
                node.second->pong_received < node.second->ping_sent &&
                now_sec.count() - node.second->ping_sent > FLAGS_cluster_node_timeout_sec/2){
                node.second->close();
                continue;
            }
        }
        // 以下代码只在节点发送了 PING 命令的情况下执行
        if (node.second->ping_sent == 0) continue;
        long long delay = now_sec.count() - node.second->ping_sent;
        if (delay > FLAGS_cluster_node_timeout_sec){
            if (!(node.second->isPFail() || node.second->isFail())){
                node.second->setFlag(clusterLink::flags::PFAIL);
                update_state = true;
            }
        }
    }
    return update_state;
}

void naruto::cluster::clusterView::checkLink() {
    auto now = system_clock::now();
    for (const auto& node : nodes){
        // 跳过当前节点
        if (node.second->isMyself()) continue;
        // 如果 handshake 节点已超时，释放它
        auto handshake_spends = duration_cast<milliseconds>(now - node.second->ctime);
        if (node.second->isHandshake()
            && handshake_spends.count() > FLAGS_cluster_handshake_timeout_ms){
            nodes.erase(node.second->addr);
            continue;
        }
        if (node.second->link) continue;

        // 为未创建连接的节点创建连接
        connection::ConnectOptions opts;
        opts.host = node.second->ip;
        opts.port = node.second->port;
        node.second->link = std::make_shared<clusterLink>(opts);
        node.second->link->regReadEvent();
        ping(node.second, cmdtype::Type::CLUSTER_MEET);
        node.second->delFlag(clusterLink::flags::MEET);
        LOG(INFO) << "Connecting with node " << node.second->addr;
    }
}

void naruto::cluster::clusterView::checkReplica() const {
    if (myself->isSlave() &&
        replica::replptr->getMasterHost().empty() &&
        myself->slaveof &&
        !myself->addr.empty()){
        replica::replptr->setMasterHost(myself->slaveof->ip);
        replica::replptr->setMasterPort(myself->slaveof->port);
    }
}

void naruto::cluster::clusterView::updateView() {
    static long long first_call_time_ms = 0;
    clusterState new_state;
    auto now_ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    if (first_call_time_ms == 0)
        first_call_time_ms = now_ms;
    // 集群写保护
    // 因为当集群节点重启后，不能够立刻可写，此时集群可能还没准备好。
    if (myself->isMaster() &&
        now_ms - first_call_time_ms < FLAGS_cluster_writable_delay_ms) return;
    new_state = clusterState::OK;
    for (int i = 0; i < CLUSTER_SLOTS; ++i) {
        if (!slots[i] || slots[i]->isFail()){
            new_state = clusterState::FAIL;
            break;
        }
    }
    int unreachable_masters = 0;
    // 统计在线并且正在处理至少一个槽的 master 的数量
    // 以及下线 master 的数量
    for(const auto& node : nodes){
        if (node.second->isMaster() && node.second->numslots > 0){
            size++;
            if (node.second->isFail() || node.second->isFail()){
                unreachable_masters++;
            }
        }
    }

    // 如果不能连接到半数以上节点，那么将我们自己的状态设置为 FAIL
    // 因为在少于半数节点的情况下，节点是无法将一个节点判断为 FAIL 的。
    int needed_quorum = (size/2) + 1;
    if (unreachable_masters >= needed_quorum){
        new_state = clusterState::FAIL;
    }
    // 记录状态变更

    // 设置新状态
    state = new_state;
}

std::shared_ptr<naruto::cluster::clusterNode> naruto::cluster::clusterView::randomNode() {
    std::default_random_engine random(time(NULL));
    std::uniform_int_distribution<int> gen(0, CLUSTER_SLOTS-1);
    int idx = 0;
    if (nodes.empty()) return nullptr;

    while (true){
        idx = gen(random);
        if (slots[idx]){
            break;
        }
    }
    return slots[idx];
}
