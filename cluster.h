//
// Created by 王振奎 on 2020/8/11.
//

#ifndef NARUTO__CLUSTER_H
#define NARUTO__CLUSTER_H

#include <ev++.h>
#include <shared_mutex>
#include <mutex>
#include <unordered_map>
#include <ctime>
#include <netinet/in.h>
#include <unistd.h>
#include <list>
#include <chrono>

#include "types.h"
#include "global.h"
#include "connection/connection.h"
#include "protocol/cluster_message.pb.h"


namespace naruto{

#define CLUSTER_BASE_PORT 10000
// 集群 slot 总数
#define CLUSTER_SLOTS 16384

// 该节点为主节点
#define CLUSTER_NODE_MASTER 1
// // 该节点为从节点
#define CLUSTER_NODE_SLAVE 2
// // 该节点疑似下线，需要对它的状态进行确认
#define CLUSTER_NODE_PFAIL 4
// // 该节点已下线
#define CLUSTER_NODE_FAIL 8
// 该节点是当前节点自身
#define CLUSTER_NODE_MYSELF 16
// 该节点还未与当前节点完成第一次 PING - PONG 通讯
#define CLUSTER_NODE_HANDSHAKE 32
// 该节点没有地址
#define CLUSTER_NODE_NOADDR 64
// 当前节点还未与该节点进行过接触
// 带有这个标识会让当前节点发送 MEET 命令而不是 PING 命令
#define CLUSTER_NODE_MEET 128
// 该节点被选中为新的主节点
#define CLUSTER_NODE_PROMOTED 256


// 空名字（在节点为主节点时，用作消息中的 slaveof 属性的值）
#define CLUSTER_NODE_NULL_NAME "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
#define CLUSTER_NODE_NAME_LEN 40

// 集群消息类型定义
#define CLUSTER_MESSAGE_TYPE_PING 0 // PING
#define CLUSTER_MESSAGE_TYPE_PONG 1 // PONG （回复 PING）
#define CLUSTER_MESSAGE_TYPE_MEET 2 // 请求将某个节点添加到集群中
#define CLUSTER_MESSAGE_TYPE_FAIL 3 // 将某个节点标记为 FAIL
#define CLUSTER_MESSAGE_TYPE_FAILOVER_AUTH_REQUEST 5 // 请求进行故障转移操作，要求消息的接收者通过投票来支持消息的发送者
#define CLUSTER_MESSAGE_TYPE_FAILOVER_AUTH_ACK 6 // 消息的接收者同意向消息的发送者投票
#define CLUSTER_MESSAGE_TYPE_SLOT_UPDATE 7 // 槽布局已经发生变化，消息发送者要求消息接收者进行相应的更新
#define CLUSTER_MESSAGE_TYPE_MF_START 8 // 为了进行手动故障转移，暂停各个客户端


struct clusterNode{
    clusterNode(const std::string ip, int port, int flags) :
                                                    slaves(), ip(ip), port(port){
        name = genName(CLUSTER_NODE_NAME_LEN);
        ctime = std::chrono::steady_clock::now();
        this->flags = flags;
        slots.resize(CLUSTER_SLOTS);
        numslots = 0;
        numslaves = 0;
        slaveof = nullptr;
        ping_sent = 0;
        pong_received = 0;
        fail_time = 0;
        voted_time = 0;
        repl_offset_time = 0;
        repl_offset = 0;
    }

    ~clusterNode(){
        if (link != nullptr) {
            link->close();
            delete link;
        }
    }

    // 保存连接节点信息
    naruto::connection::Connect* link;

    // Node object creation time
    std::chrono::steady_clock::time_point ctime;
//    mstime_t ctime;

    // 节点的名字，由 40 个十六进制字符组成
    // 例如 68eef66df23420a5862208ef5b1a7005b806f2ff
    std::string name;

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
    std::vector<bool> slots;
//    unsigned  char slots[CLUSTER_SLOTS/8];

    // 该节点负责处理的槽数量
    int numslots;

    // 如果本节点是主节点，那么用这个属性记录从节点的数量
    int numslaves;

    // 指针数组，指向各个从节点
    std::list<std::shared_ptr<clusterNode>> slaves;

    // 如果这是一个从节点，那么指向主节点
    std::shared_ptr<clusterNode> slaveof;

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
    std::string ip;

    // 节点的端口号
    int port;

    // 一个链表，记录了所有其他节点对该节点的下线报告
    std::list<void*>* fail_reports;

    bool isMaster(){ return (flags & CLUSTER_NODE_MASTER) > 0; }
    bool isSlave() { return (flags & CLUSTER_NODE_SLAVE) > 0; }
    bool isMyself() { return (flags & CLUSTER_NODE_MYSELF) > 0; }
    bool inHandshake() { return (flags & CLUSTER_NODE_HANDSHAKE) > 0; }
    bool noAddr() { return (flags & CLUSTER_NODE_NOADDR) > 0;}
    bool isTimeout() { return (flags & CLUSTER_NODE_PFAIL) > 0; }
    bool isFail() { return (flags & CLUSTER_NODE_FAIL) > 0; }

    static std::string genName(int length){
        std::string result;
        result.resize(length);

        srand(time(NULL));
        for (int i = 0; i < length; ++i) {
            result[i] = charset[rand() % charset.length()];
        }
        return result;
    }
};


class Cluster{
public:
    explicit Cluster(int port, int tcp_backlop);
    ~Cluster();

    void onAccept(ev::io&, int);
    void onEvents(ev::io& watcher, int events);
    void onPing(ev::timer& watcher, int events);

    void run();
    void stop();

private:

    void _remove_node(const std::string& name);
    void _remove_slave_of_master(std::shared_ptr<clusterNode> master, std::shared_ptr<clusterNode> slave);

    // 向指定节点发送一条 MEET 、 PING 或者 PONG 消息
    void _send_ping(std::shared_ptr<clusterNode> node, int type);

    int _fd;
    int _port;
    int _tcp_backlog;
    int _connect_nums;
    std::shared_ptr<ev::io> _cluster_ev_io_w;
    std::shared_ptr<ev::io> _cluster_ev_io_r;

    std::chrono::milliseconds _cluster_node_timeout;

    // 指向当前节点的指针
    std::shared_ptr<clusterNode> _myself;

    // 存储集群节点 key 为 nodename
    std::unordered_map<std::string, std::shared_ptr<clusterNode>> _nodes;

    // 集群当前的配置纪元，用于实现故障转移
    uint64_t _currentEpoch;

    // 集群当前的状态：是在线还是下线
    int _state;

};

}


#endif //NARUTO__CLUSTER_H
