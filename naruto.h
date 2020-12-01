//
// Created by 王振奎 on 2020/8/14.
//

#ifndef NARUTO__NARUTO_H
#define NARUTO__NARUTO_H

#include <ev++.h>
#include <string>
#include <list>
#include <gflags/gflags.h>
#include <unistd.h>
#include <netinet/in.h>
#include <deque>
#include <glog/logging.h>
#include <cstring>
#include <arpa/inet.h>
#include <float.h>
#include <math.h>
#include <sys/wait.h>
#include <sys/types.h>


#include "types.h"
#include "cluster.h"
#include "utils/net.h"
#include "replication.h"
#include "connect_worker.h"

namespace naruto{

class Naruto{
public:
    void initializer();
    void onAccept(ev::io&, int);
    void onAcceptRc(ev::io&, int);
    static void onSignal(ev::sig&, int);
    void onCron(ev::timer&, int);
    void start();

    ~Naruto();
private:
    void _init_workers();
    void _init_cron();
    void _init_cluster();
    void _init_signal();
    void _listen();

    // 处理信号，任务线程执行频率
    std::string run_id_;
    bool cluster_enable_;
    ev::io ct_rio_;
    ev::io rc_rio_;

    ev::sig sigint_;
    ev::sig sigterm_;
    ev::sig sigkill_;
    ev::timer timer_watcher_;
    std::shared_ptr<Cluster> cluster_;

    // TCP
    int fd_;
    int rc_fd_;
    int tcp_backlog_;

    bool clients_paused_; // 暂停客户端
    mstime_t clients_pause_end_time_;

    // stat
    int connect_nums_;
    // 服务器启动时间
    time_t stat_start_time_;
    // 已处理命令的数量
    long long stat_num_commands_;
    // 服务器接到的连接请求数量
    long long stat_num_connections_;
    // 已过期的键数量
    long long stat_expire_keys_;
    // 成功查找键的次数
    long long stat_keyspace_hits_;
    // 查找键失败的次数
    long long stat_keyspace_miss_;
    // 已使用内存峰值
    size_t stat_peak_memory_;
};

extern Naruto* server;

}

#endif //NARUTO__NARUTO_H
