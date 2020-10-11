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

#include "types.h"
#include "connect_worker.h"
#include "cluster.h"
#include "database/buckets.h"
#include "command/command.h"
#include "command/commands.h"
#include "command/command_nf.h"
#include "command/command_hget.h"
#include "utils/net.h"

//DEFINE_string(host,"","host");

namespace naruto{

class Naruto{
public:
    explicit Naruto(int port, int tcp_backlog, int bucket_num);

    ~Naruto();

    void onAccept(ev::io&, int);
    static void onSignal(ev::sig&, int);

    // 向集群中的所有断线或者未连接节点发送消息
    // 遍历所有节点，检查是否需要将某个节点标记为下线
    void onCron(ev::timer&, int);
    void run();

private:

    void _init_workers();
    void _init_cluster();
    void _init_signal();
    void _listen();

    // database
    int _bucket_num;

    // 处理信号，任务线程执行频率
    int _hz;
    int _cron_loops;

//    std::shared_ptr<command::Commands> _commands;
//    std::shared_ptr<database::Buckets> _buckets;

    bool _cluster_enable;
    ev::io _accept_watcher;
    ev::sig _sigint;
    ev::sig _sigterm;
    ev::sig _sigkill;
    ev::timer _timer_watcher;
    ev::default_loop _loop;
    Cluster _cluster;

    // 本服务器运行时的ID，每次重启都会变化，运行中则不会变
    // 唯一标识一个运行中的实例
    std::string _run_id;

    // TCP
    int _port;
    int _fd;
    int _tcp_backlog;
    std::string _bind_addr;
    int _bind_add_count;

    // Client
    // 一个链表，保存了所有客户端状态结构
    std::list<std::shared_ptr<narutoClient>> _clents;

    // 链表，保存了所有待关闭的客户端
    std::list<std::shared_ptr<narutoClient>> _slaves;

    bool _clients_paused; // 暂停客户端
    mstime_t _clients_pause_end_time;

    // RDB / AOF
    bool _loading;
    off_t _loading_total_bytes;
    off_t _loading_loaded_bytes;

    time_t _loading_start_time;
    off_t _loading_process_events_interval_bytes;

    // stat
    int _connect_nums;
    // 服务器启动时间
    time_t _stat_start_time;
    // 已处理命令的数量
    long long _stat_num_commands;
    // 服务器接到的连接请求数量
    long long _stat_num_connections;
    // 已过期的键数量
    long long _stat_expire_keys;
    // 成功查找键的次数
    long long _stat_keyspace_hits;
    // 查找键失败的次数
    long long _stat_keyspace_miss;
    // 已使用内存峰值
    size_t _stat_peak_memory;
    // 服务器因为客户端数量过多而拒绝客户端连接的次数
    long long _stat_rejected_conn;
    // 执行 full sync 的次数
    long long _stat_sync_full;
    // PSYNC 成功执行的次数
    long long _stat_sync_partial_ok;
    // PSYNC 执行失败的次数
    long long _stat_sync_partial_err;
};

}

#endif //NARUTO__NARUTO_H
