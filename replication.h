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
#include <array>
#include <atomic>
#include <algorithm>
#include <cstring>

#include "client.h"
#include "defines.h"
#include "utils/bytes.h"
#include "database/buckets.h"
#include "connection/connection.h"
#include "utils/pack.h"
#include "sink/rotate_file_stream.h"
#include "protocol/replication.pb.h"
#include "command/commands.h"

namespace naruto{

struct saveparam{
    // 多少秒之内
    std::chrono::milliseconds ms;

    // 发生多少次修改
    int changes;
};

struct replConf{
    std::string aof_file_name;
    uint64_t  aof_off_set;

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

class Replication{
public:
    Replication(int worker_num);
    const std::string &getMasterHost() const;
    void setMasterHost(const std::string &masterHost);
    int getMasterPort() const;
    void setMasterPort(int masterPort);
    bool isIsMaster() const;
    void setIsMaster(bool isMaster);
    state getReplState() const;
    void setReplState(state replState);

    void onReadSyncBulkPayload(ev::io&, int);
    void onSyncWithMaster(ev::io&, int);
    void onBgsaveFinish(ev::child& child, int events);
    void onReplCron();
    void onReplConfFlush();
    void backlogFeed(int tid, const ::google::protobuf::Message&, uint16_t type);
    void backlogFeed(int tid, utils::Bytes& pack);
    void bgsave();
    static void databaseLoad();
    ~Replication();

private:
    struct indexLog{
        int tid; // 线程id
        uint64_t id; // 排序id
    };
    void _backlog_feed(const unsigned char* data, size_t size);
    void _backlog_flush();
    void _merge_backlog_feed_slaves();
    void _undo_connect_master();
    void _connect_master();
    void _repl_cache_master();
    void _repl_discard_cache_master();
    void _abort_sync_transfer();
    void _free_client(std::shared_ptr<narutoClient>&);
    void _repl_send_ack();
    void _remove_bgsave_tmp_file(pid_t childpid);
    replication::type _slave_try_partial_resynchronization(int fd);
    std::string _bgsave_file_name(pid_t pid);

    uint64_t cronloops_;
    bool cron_run_;
    double cron_interval_;
    ev::timer time_watcher_;
    bool is_master_;
    std::chrono::steady_clock::time_point repl_unixtime_;
    std::chrono::steady_clock::time_point repl_no_slaves_since_;
    std::chrono::steady_clock::time_point repl_down_since_;


    std::chrono::steady_clock::time_point last_flush_aof_time_;
    int dirty_;
    int dirty_before_bgsave_;
    long long changes_;
    std::list<saveparam> saveparams_;

    // bgsave dump db
    int bgsave_child_pid_; // bgsave 子进程 pid
    std::chrono::milliseconds bgsave_fork_spends_; // bgsave fork 耗时
    std::chrono::steady_clock::time_point bgsave_time_start_; // bgsave开始时间
    std::chrono::milliseconds bgsave_time_last_; // bgsave 耗时
    std::chrono::steady_clock::time_point bgsave_last_try_; // 最后一次执行时间
    std::string bgsave_aof_filename_; // 执行bgsave是 aof 文件名
    int64_t bgsave_aof_off; // 执行bgsave时 aof offset
    int bgsave_last_status_; // bgsave 最后一次执行状态

    bool use_aof_checksum_;

    std::shared_ptr<narutoClient> master_; // connect to master
    std::shared_ptr<narutoClient> cache_master_; // 为了实现 当正常同步时，master 连接中断,当尝试重连时，可以顺利执行 部分重同步

    std::list<std::shared_ptr<narutoClient>> slaves_;

    // replication
    std::atomic_int repl_pos_; // 当前使用的index

    int repl_merge_size_; // 和 worker 数一致
    using repl_workers = std::vector<naruto::utils::Bytes>;
    std::array<std::shared_ptr<repl_workers>,2> repl_;

    std::atomic_uint64_t repl_command_incr_; // 命令自增数
    std::vector<uint8_t> back_log_;
    std::shared_ptr<sink::RotateFileStream> aof_file_;

    // master
    // 全局复制偏移量（一个累计值）
    long long master_repl_offset_;
    // 已经flush到aof文件的大小
    long long master_flush_repl_offset_;
    int repl_ping_slave_period_;
    // 环形缓冲长度
    long long repl_back_size_;

    // slave
    std::string master_host_;
    int master_port_;
    enum state repl_state_;
    std::shared_ptr<ev::io> repl_ev_io_w_;
    std::shared_ptr<ev::io> repl_ev_io_r_;
    off_t repl_database_size; // 主服务器传来的 RDB 文件的大小
    off_t repl_database_synced_size; // 已读取主服务器传来的 RDB 文件的大小
    int repl_timeout_;

    // RDB 文件的大小
    off_t repl_transfer_size_;
    // 已读 RDB 文件内容的字节数
    off_t repl_transfer_read_;
    // 最近一次执行 fsync 时的偏移量
    off_t repl_transfer_last_fsync_off_;
    // 主服务器的套接字
    int repl_transfer_s_;
    // 保存 RDB 文件的临时文件的描述符
    int repl_transfer_fd_;
    // 保存 RDB 文件的临时文件名字
    std::string repl_transfer_tmpfile_;
    // 最近一次读入 RDB 内容的时间
    std::chrono::steady_clock::time_point repl_transfer_lastio_;
    std::chrono::steady_clock::time_point repl_last_interaction_;
    // 是否只读从服务器？
    bool repl_slave_ro_;
    // 连接断开的时长
    std::string repl_master_runid_;
    // 初始化偏移量
    long long repl_master_initial_offset_;

};


}



#endif //NARUTO_REPLICATION_H
