//
// Created by 王振奎 on 2020/8/11.
//

#ifndef NARUTO__CLUSTER_NODE_H
#define NARUTO__CLUSTER_NODE_H

#include <list>
#include <netinet/in.h>
#include <unistd.h>
#include <unordered_map>
#include "types.h"

// 槽数量
#define CLUSTER_SLOTS 16384
#define CLUSTER_OK 0
#define CLUSTER_FAIL 1

// 集群名字长度，sha1 hex length
#define CLUSTER_NAME_LEN 40

#define CLUSTER_IP_STR_LEN INET6_ADDRSTRLEN

// 空名字（在节点为主节点时，用作消息中的 slaveof 属性的值）
#define CLUSTER_NODE_NULL_NAME "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"

// 集群消息类型定义
#define CLUSTER_MESSAGE_TYPE_PING 0 // PING
#define CLUSTER_MESSAGE_TYPE_PONG 1 // PONG （回复 PING）
#define CLUSTER_MESSAGE_TYPE_MEET 2 // 请求将某个节点添加到集群中
#define CLUSTER_MESSAGE_TYPE_FAIL 3 // 将某个节点标记为 FAIL
#define CLUSTER_MESSAGE_TYPE_FAILOVER_AUTH_REQUEST 5 // 请求进行故障转移操作，要求消息的接收者通过投票来支持消息的发送者
#define CLUSTER_MESSAGE_TYPE_FAILOVER_AUTH_ACK 6 // 消息的接收者同意向消息的发送者投票
#define CLUSTER_MESSAGE_TYPE_SLOT_UPDATE 7 // 槽布局已经发生变化，消息发送者要求消息接收者进行相应的更新
#define CLUSTER_MESSAGE_TYPE_MF_START 8 // 为了进行手动故障转移，暂停各个客户端


// 前置定义，防止编译错误
struct clusterNode;

struct clusterLink{
    // 连接的创建时间
    mstime_t ctime;

    // TCP 套接字描述符
    int fd;

    // 输出缓冲区，保存着等待发送给其他节点的消息（message）。
    string* sndbuf;

    // 输入缓冲区，保存着从其他节点接收到的消息。
    string* rcvbuf;

    // 与这个连接相关联的节点，如果没有的话就为 NULL
    clusterNode* node;
};

struct clusterNode{
    // Node object creation time
    mstime_t ctime;

    // 节点的名字，由 40 个十六进制字符组成
    // 例如 68eef66df23420a5862208ef5b1a7005b806f2ff
    string name;

    // 节点标识
    // 使用各种不同的标识值记录节点的角色（比如主节点或者从节点），
    // 以及节点目前所处的状态（比如在线或者下线）。
    int flags;

    // 节点当前的配置纪元，用于实现故障转移
    uint64_t config_epoch;

    // 由这个节点负责处理的槽
    // 一共有 REDIS_CLUSTER_SLOTS / 8 个字节长
    // 每个字节的每个位记录了一个槽的保存状态
    // 位的值为 1 表示槽正由本节点处理，值为 0 则表示槽并非本节点处理
    // 比如 slots[0] 的第一个位保存了槽 0 的保存情况
    // slots[0] 的第二个位保存了槽 1 的保存情况，以此类推
    unsigned  char slots[CLUSTER_SLOTS/8];

    // 该节点负责处理的槽数量
    int numslots;

    // 如果本节点是主节点，那么用这个属性记录从节点的数量
    int numslaves;

    // 指针数组，指向各个从节点
    clusterNode **slaves;

    // 如果这是一个从节点，那么指向主节点
    clusterNode* slaveof;

    // 最后一次发送 PING 命令的时间
    mstime_t ping_sent;

    // 最后一次接收 PONG 回复的时间戳
    mstime_t pong_received;

    // 最后一次被设置为 FAIL 状态的时间
    mstime_t fail_time;

    // 最后一次给某个从节点投票的时间
    mstime_t voted_time;

    // 最后一次从这个节点接收到复制偏移量的时间
    mstime_t repl_offset_time;

    // 这个节点的复制偏移量
    long long repl_offset;

    // 节点的 IP 地址
    string ip;

    // 节点的端口号
    int port;

    // 保存连接节点所需的有关信息
    clusterLink* link;

    // 一个链表，记录了所有其他节点对该节点的下线报告
    std::list<void*>* fail_reports;
};

// 集群状态，每个节点都保存着一个这样的状态，记录了它们眼中的集群的样子。
// 另外，虽然这个结构主要用于记录集群的属性，但是为了节约资源，
// 有些与节点有关的属性，比如 slots_to_keys 、 failover_auth_count
// 也被放到了这个结构里面。
struct clusterState{
    // 集群当前的配置纪元，用于实现故障转移
    uint64_t current_epoch;

    // 集群当前的状态
    int state;

    // 集群中至少处理着一个槽的节点数量
    int size;

    // 集群节点名单（包括 myself 节点）
    // 字典的键为节点的名字，字典的值为 clusterNode 结构
    std::unordered_map<std::string, clusterNode*> nodes;

    // 节点黑名单，用于 CLUSTER FORGET 命令
    std::unordered_map<std::string, int> nodes_black_list;

    // 记录要从当前节点迁移到目标节点的槽，以及迁移的目标节点
    // migrating_slots_to[i] = NULL 表示槽 i 未被迁移
    // migrating_slots_to[i] = clusterNode_A 表示槽 i 要从本节点迁移至节点 A
    clusterNode* migrating_slots_to[CLUSTER_SLOTS];

    // 记录要从源节点迁移到本节点的槽，以及进行迁移的源节点
    // importing_slots_from[i] = NULL 表示槽 i 未进行导入
    // importing_slots_from[i] = clusterNode_A 表示正从节点 A 中导入槽 i
    clusterNode* migrating_slots_from[CLUSTER_SLOTS];

    // 负责处理各个槽的节点
    // 例如 slots[i] = clusterNode_A 表示槽 i 由节点 A 处理
    clusterNode* slots[REDIS_CLUSTER_SLOTS];


    // 跳跃表，表中以槽作为分值，键作为成员，对槽进行有序排序
    // 当需要对某些槽进行区间（range）操作时，这个跳跃表可以提供方便
    // TODO:

    // 以下这些域被用于进行故障转移选举

    // 上次执行选举或者下次执行选举的时间
    mstime_t failover_auth_time;

    // 节点获得的投票数量
    int failover_auth_count;

    // 如果值为 1 ，表示本节点已经向其他节点发送了投票请求
    int failover_auth_sent;

    int failover_auth_rank;

    // 本次选举的纪元
    uint64_t failover_auth_epoch;

    // 共用的手动故障转移状态
    // 手动故障转移执行的时间限制
    mstime_t mf_end;

    // 主服务器的手动故障转移状态
    clusterNode* mf_slave;

    // 从服务器的手动故障转移状态
    long long mf_master_offset;

    // 指示手动故障转移是否可以开始的标志值
    // 值为非 0 时表示各个主服务器可以开始投票
    int mf_can_start;


    // 以下这些域由主服务器使用，用于记录选举时的状态
    // 集群最后一次进行投票的纪元
    uint64_t  last_vote_epoch;

    // 在进入下个事件循环之前要做的事情，以各个 flag 来记录
    int todo_before_sleep;

    // 通过 cluster 连接发送的消息数量
    long long stats_bus_messages_sent;

    // 通过 cluster 接收到的消息数量
    long long stats_bus_messages_received;
};

// =================== 集群消息 =============================
struct clusterMsgDataGossip{
    // 节点的名字
    // 在刚开始的时候，节点的名字会是随机的
    // 当 MEET 信息发送并得到回复之后，集群就会为节点设置正式的名字
    char nodename[CLUSTER_NAME_LEN];

    // 最后一次向该节点发送 PING 消息的时间戳
    uint32_t ping_sent;

    // 最后一次从该节点接收到 PONG 消息的时间戳
    uint32_t pong_received;

    // 节点的 IP 地址
    char ip[CLUSTER_IP_STR_LEN];

    // 节点的端口号
    uint16_t port;

    // 节点的标识值
    uint16_t flags;

    // 对齐字节，不使用
    uint32_t notused;
};

struct clusterMsgDataFail{
    // 下线节点的名字
    char nodename[CLUSTER_NAME_LEN];
};

struct clusterMsgDataUpdate{
    // 节点的配置纪元
    uint64_t config_epoch;

    // 节点的名字
    char nodename[CLUSTER_NAME_LEN];

    // 节点的槽布局
    unsigned char slots[CLUSTER_SLOTS/8];
};

struct clusterMsgData{
    // PING, MEET and PONG
    struct{
        clusterMsgDataGossip gossip[1];
    } ping;

    // FAIL
    struct {
        clusterMsgDataFail about;
    } fail;

    // UPDATE
    struct {
        clusterMsgDataUpdate nodecfg;
    } update;
};

struct clusterMsg{
    // Siganture "RCmb" (Redis Cluster message bus).
    char sig[4];

    // 消息的长度（包括这个消息头的长度和消息正文的长度）
    uint32_t totlen;

    // 协议版本，当前为 0
    uint16_t ver;

    // 填充2字节
    uint16_t  notused0;

    // 消息类型
    uint16_t type;

    // 消息正文包含的节点信息数量
    // 只在发送 MEET 、 PING 和 PONG 这三种 Gossip 协议消息时使用
    uint16_t count;

    // 消息发送者的配置纪元
    uint16_t current_epoch;

    // 如果消息发送者是一个主节点，那么这里记录的是消息发送者的配置纪元
    // 如果消息发送者是一个从节点，那么这里记录的是消息发送者正在复制的主节点的配置纪元
    uint64_t config_epoch;

    // 节点的复制偏移量
    // 如果节点是主节点，则为主复制偏移量；如果节点为从属节点，则为已处理复制偏移量
    uint64_t offset;

    // 消息发送者的名字（ID）
    char sender[CLUSTER_NAME_LEN];

    // 消息发送者目前的槽指派信息
    unsigned char myslots[CLUSTER_SLOTS/8];

    // 如果消息发送者是一个从节点，那么这里记录的是消息发送者正在复制的主节点的名字
    // 如果消息发送者是一个主节点，那么这里记录的是 CLUSTER_NODE_NULL_NAME
    char slaveof[CLUSTER_NAME_LEN];

    // 32 bytes reserved for future usage
    char notused1[32];

    // 消息发送者的端口号
    uint16_t  port;

    // 消息发送者的标识值
    uint16_t flags;

    // 消息发送者所处集群的状态
    unsigned char state;

    // 消息标志
    // CLUSTERMSG_FLAG[012]_...
    unsigned char mflags[3];

    // 消息的正文（或者说，内容）
    union clusterMsgData data;
};

#define CLUSTER_MESSAGE_MIN_LEN (sizeof(clusterMsg) - sizeof(union clusterMsgData))

#endif //NARUTO__CLUSTER_NODE_H
