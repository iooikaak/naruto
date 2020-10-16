//
// Created by 王振奎 on 2020/9/18.
//

#ifndef NARUTO_REPLICATION_H
#define NARUTO_REPLICATION_H

#include <vector>
#include <atomic>
#include <ev++.h>
#include <list>
#include <glog/logging.h>
#include <sys/socket.h>

#include "utils/bytes.h"
#include "connection/connection.h"
#include "client.h"

namespace naruto{

struct saveparam{
    // 多少秒之内
    std::chrono::seconds seconds;

    // 发生多少次修改
    int changes;
};

enum class state{
    NONE = 0,          // 未开始复制
    CONNECT,           // 准备和 master 建立连接
    CONNECTING,        // 已经和 master 建立连接，未进行通信确认
    RECEIVE_PONG,      // 发送 Ping 到 master ，准备接受 master Pong 确认
    TRANSFOR,          // 已经和master建立连接，并且 PING-PONG 确认连接有效
    CONNECTED,         // 首次全量同步已经完成，和master连接正常连接，增量同步
    WAIT_BGSAVE_START, // 主服务器 save db 开始
    WAIT_BGSAVE_END,   // 主服务器 save db 完成
    SEND_BULK,         // 主服务器 正在 发送 db 到 slave
    ONLINE,            // 全量db 文件已经发送完成，只需要增量更新即可
};

class Replication {
public:
    Replication(int back_log_size, int merge_threads_size, int repl_timeout_sec);
    void onEvents(ev::io&, int);
    void onSyncWithMaster(ev::io&, int);

    ~Replication();

    void onReplCron(ev::timer&, int);

    void feedSlaves(const ::google::protobuf::Message&, uint16_t type);


private:
    void _feed_backlog(utils::Bytes&);
    void _undo_connect_master();
    void _connect_master();
    void _repl_cache_master();
    void _repl_discard_cache_master();
    void _abort_sync_transfer();
    void _free_client(std::shared_ptr<narutoClient>&);
    void _repl_send_ack();

    int _slave_try_partial_resynchronization();

    uint64_t _cronloops;
    double cron_interval_;
    ev::timer time_watcher_;
    // 每个线程会持有一个 写命令的 双 buffer
    using repl_bytes = std::vector<std::shared_ptr<naruto::utils::Bytes>>;
    bool _is_master;

    // 消息自增id，用于多线程命名重排序
    std::atomic_uint64_t _repl_incr_id;
    std::chrono::steady_clock::time_point _repl_unixtime;
    std::chrono::steady_clock::time_point _repl_no_slaves_since;
    std::chrono::steady_clock::time_point _repl_down_since;
    // dump db
    // 负责执行 dump db 的 子进程 id
    pid_t _dump_db_child_pid;
    std::list<saveparam> _saveparams;

    // connect to master
    std::shared_ptr<narutoClient> _master;
    // 为了实现 当正常同步时，master 连接中断
    // 当尝试重连时，可以顺利执行 部分重同步
    std::shared_ptr<narutoClient> _cache_master;
    std::list<std::shared_ptr<narutoClient>> _slaves;

    std::shared_ptr<ev::io> _repl_ev_io_w;
    std::shared_ptr<ev::io> _repl_ev_io_r;

    // 当前使用的index
    std::atomic_int _pos;
    std::vector<repl_bytes> _repl;

    // replication
    std::vector<uint8_t> _back_log;

    // master
    std::string _master_host;
    int _master_port;
    // 全局复制偏移量（一个累计值）
    long long _master_repl_offset;
    int _repl_ping_slave_period;
    // 环形缓冲长度
    long long _repl_back_size;
    // backlog 中数据的长度(实际存储数据)
    long long _repl_backlog_histlen;
    // backlog 的当前索引
    long long _repl_backlog_idx;
    // backlog 中可以被还原的第一个字节的偏移量
    // 即可读的第一个位置
    long long _repl_backlog_off;

    // slave
    // 复制状态
//    int _repl_state;
    enum state _repl_state;

    int _repl_timeout;
    // RDB 文件的大小
    off_t _repl_transfer_size;
    // 已读 RDB 文件内容的字节数
    off_t _repl_transfer_read;
    // 最近一次执行 fsync 时的偏移量
    off_t _repl_transfer_last_fsync_off;
    // 主服务器的套接字
    int _repl_transfer_s;
    // 保存 RDB 文件的临时文件的描述符
    int _repl_transfer_fd;
    // 保存 RDB 文件的临时文件名字
    std::string _repl_transfer_tmpfile;
    // 最近一次读入 RDB 内容的时间
    std::chrono::steady_clock::time_point _repl_transfer_lastio;
    std::chrono::steady_clock::time_point _repl_last_interaction;
    // 是否只读从服务器？
    bool _repl_slave_ro;
    // 连接断开的时长
    std::string _repl_master_runid;
    // 初始化偏移量
    long long _repl_master_initial_offset;

};


}



#endif //NARUTO_REPLICATION_H
