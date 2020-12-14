//
// Created by kwins on 2020/12/8.
//

#ifndef NARUTO_CLUSTER_NODE_H
#define NARUTO_CLUSTER_NODE_H
#include <chrono>
#include <string>
#include <list>
#include <utility>
#include "cluster_const.h"
#include "cluster_link.h"
#include "protocol/command_types.pb.h"
#include "protocol/cluster_message.pb.h"

namespace naruto::cluster {

using namespace std::chrono;
class clusterNode;

struct clusterFailReport{
    // 报告目标节点已经下线的节点
    std::shared_ptr<clusterNode> node;

    // 最后一次从 node 节点收到下线报告的时间
    // 程序使用这个时间戳来检查下线报告是否过期
    system_clock::time_point time;
};

struct clusterNode {
    clusterNode(const std::string& ip, int port,uint32_t flags){
        this->name = "";
        this->flags = flags;
        this->ctime = system_clock::now();
        this->config_epoch = 0;
        this->numslots = 0;
        this->numslaves = 0;
        this->slaveof = nullptr;
        this->repl_offset = 0;
        this->ip = ip;
        this->port = port;
        this->addr = ip + ":" + std::to_string(port);
        this->port = 0;
        this->link = nullptr;
    }

    bool isSlave() const { return flags & (unsigned) clusterLink::flags::SLAVE; }
    bool isMaster() const { return flags & (unsigned) clusterLink::flags::MASTER; }
    bool isMyself() const { return flags & (unsigned) clusterLink::flags::MYSELF; }
    bool isPFail() const { return flags & (unsigned) clusterLink::flags::PFAIL; }
    bool isFail() const { return flags & (unsigned) clusterLink::flags::FAIL; }
    bool isHandshake() const { return flags & (unsigned) clusterLink::flags::HANDSHAKE; }
    bool isMeet() const { return flags & (unsigned) clusterLink::flags::MEET; }
    void setFlag(clusterLink::flags flag){ flags |= (unsigned) flag; }
    void delFlag(clusterLink::flags flag) {flags &= ~(unsigned) flag; }
    void desc(::cluster::command_node_data& info);

    void close(){
        if (!link) return;
        link->c_wio.stop();
        if (link->c_rio) link->c_rio->stop();
        if (link){ link->close();}
        link = nullptr;
    }

    std::string addr;
    // 创建节点的时间
    system_clock::time_point ctime;
    // 节点的名字，由 40 个十六进制字符组成
    // 例如 68eef66df23420a5862208ef5b1a7005b806f2ff
    std::string name;
    // 节点标识
    // 使用各种不同的标识值记录节点的角色（比如主节点或者从节点），
    // 以及节点目前所处的状态（比如在线或者下线）。
    uint32_t flags;
    // 节点当前的配置纪元，用于实现故障转移
    uint64_t config_epoch;

    // 由这个节点负责处理的槽
    // 一共有 CLUSTER_SLOTS 个字节长
    // 每个字节的每个位记录了一个槽的保存状态
    // 位的值为 1 表示槽正由本节点处理，值为 0 则表示槽并非本节点处理
    // 比如 slots[0] 的第一个位保存了槽 0 的保存情况
    // slots[0] 的第二个位保存了槽 1 的保存情况，以此类推
    bool slots[CLUSTER_SLOTS]{};

    // 该节点负责处理的槽数量
    int numslots;
    // 如果本节点是主节点，那么用这个属性记录从节点的数量
    int numslaves;
    // 指针数组，指向各个从节点
    std::list<std::shared_ptr<clusterNode>> slaves{};
    // 如果这是一个从节点，那么指向主节点
    std::shared_ptr<clusterNode> slaveof;
    // 最后一次发送 PING 命令的时间
    long long ping_sent = 0;
    // 最后一次接收 PONG 回复的时间戳, 毫秒
    long long pong_received{};
    // 最后一次被设置为 FAIL 状态的时间
    long long fail_time{};
    // 最后一次给某个从节点投票的时间
    long long voted_time{};
    // 最后一次从这个节点接收到复制偏移量的时间
    long long repl_offset_time{};
    // 这个节点的复制偏移量
    std::string repl_aof_name;
    long long repl_offset;
    // 节点的 IP 地址
    std::string ip;
    // 节点的端口号
    int port;
    std::shared_ptr<clusterLink> link;
    // 一个链表，记录了所有其他节点对该节点的下线报告
    std::list<clusterFailReport> fail_reports{};
};

}

#endif //NARUTO_CLUSTER_NODE_H
