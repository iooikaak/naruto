//
// Created by kwins on 2020/12/8.
//

#ifndef NARUTO_CLUSTER_VIEW_H
#define NARUTO_CLUSTER_VIEW_H

#include <unordered_map>
#include <array>
#include <list>

#include "cluster_node.h"

namespace naruto::cluster {

enum class clusterState{
    OK = 0,
    FAIL = 1,
};

struct clusterView {
    // 发送gossip消息
    void ping(const std::shared_ptr<clusterNode>& node, cmdtype::Type type);
    void gossip();
    void checkLink();
    bool checkOffline();
    void checkReplica() const;
    void updateView();
    std::shared_ptr<clusterNode> randomNode();

    // 集群上线时间
    system_clock::time_point online_time;
    // 指向当前节点的指针
    std::shared_ptr<clusterNode> myself;
    // 集群当前的配置纪元，用于实现故障转移
    uint64_t current_epoch;
    // 集群当前的状态：是在线还是下线
    clusterState state;
    // 集群中至少处理着一个槽的节点的数量。
    // 即有效节点数量
    int size;
    std::unordered_map<std::string, std::shared_ptr<clusterNode>> nodes;
    // 记录要从当前节点迁移到目标节点的槽，以及迁移的目标节点
    // 通过 slots 可以找到对应 clusterNode
    bool migrating_slots_to[CLUSTER_SLOTS];
    // 记录要从源节点迁移到本节点的槽，以及进行迁移的源节点
    // 通过 slots 可以找到对应 clusterNode
    bool importing_slots_from[CLUSTER_SLOTS];
    // 负责处理各个槽的节点
    // 例如 slots[i] = clusterNode_A 表示槽 i 由节点 A 处理
    std::array<std::shared_ptr<clusterNode>, CLUSTER_SLOTS> slots;

    // 以下这些域被用于进行故障转移选举

    // 上次执行选举或者下次执行选举的时间
    system_clock::time_point failover_auth_time;
    // 节点获得的投票数量
    int failover_auth_count;
    // 如果值为 true ，表示本节点已经向其他节点发送了投票请求
    bool failover_auth_sent;
    //
    int failover_auth_rank;
    // 当前选举的纪元
    uint64_t failover_auth_epoch;

    // 以下这些域由主服务器使用，用于记录选举时的状态
    // 集群最后一次进行投票的纪元
    uint64_t last_vote_epoch;
    // 通过 cluster 连接发送的消息数量
    long long stats_bus_messages_sent;
    // 通过 cluster 接收到的消息数量
    long long stats_bus_messages_received;
};

}

#endif //NARUTO_CLUSTER_VIEW_H
