//
// Created by kwins on 2020/12/8.
//

#ifndef NARUTO_REPLICATION_H
#define NARUTO_REPLICATION_H

#include <string>
#include <ev++.h>
#include <list>
#include <google/protobuf/message.h>
#include "replica_state.h"
#include "replica_link.h"
#include "sink/rotate_file_stream.h"

namespace naruto::replica {
using namespace std::chrono;

class Replication {
public:
    ~Replication() = default;

    void initializer();
    const std::string &getMasterHost() const;
    void setMasterHost(const std::string &masterHost);
    int getMasterPort() const;
    void setMasterPort(int masterPort);
    bool isIsMaster() const;
    void setIsMaster(bool isMaster);
    replState getReplState() const;
    void setReplState(replState replState);
    replConf &getReplConf();

    void onReadSyncBulkPayload(ev::io&, int);
    void onSyncWithMaster(ev::io&, int);
    void onBgsaveFinish(ev::child& child, int events);

    void onBgsave(ev::timer& watcher, int event);
    void bgsave();
    void onReplCron(ev::timer& watcher, int event);
    void onReplConfFlushCron(ev::timer& watcher, int event);
    void replConfFlush();
    void backlogFeed(int tid, const ::google::protobuf::Message&, uint16_t type);
    void backlogFeed(int tid, const unsigned char* s, size_t  n, uint16_t type);
    void backlogFeed(int tid, utils::Bytes& pack);
    void addSlave(replicaLink*);

    void cacheMaster();
    void discardCacheMaster();
    void statSlave(); // close slave 时记录一些统计数据
    void stop();

private:
    struct indexLog{
        int tid; // 线程id
        uint64_t id; // 排序id
    };

    void _init();
    void _backlog_feed(const unsigned char* data, size_t size);
    void _backlog_flush();
    void _merge_backlog_feed_slaves();
    void _undo_connect_master();
    void _connect_master();

    void _abort_sync_transfer();
    void _heartbeat();
    void _remove_bgsave_tmp_file(pid_t childpid);
    int _slave_try_partial_resynchronization(int fd);
    std::string _bgsave_file_name(pid_t pid);

    uint64_t cronloops_;
    ev::timer flush_watcher_;
    ev::timer cron_watcher_;
    ev::timer bgsave_watcher_;

    bool is_master_;
    std::chrono::steady_clock::time_point repl_unixtime_;
    std::chrono::system_clock::time_point repl_no_slaves_since_;
    std::chrono::steady_clock::time_point repl_down_since_;
    std::chrono::steady_clock::time_point last_flush_aof_time_;

    int dirty_;
    int dirty_before_bgsave_;
    long long changes_;
//    std::list<saveparam> saveparams_;

    // bgsave dump db
    int bgsave_child_pid_; // bgsave 子进程 pid
    std::chrono::milliseconds bgsave_fork_spends_; // bgsave fork 耗时
    std::chrono::steady_clock::time_point bgsave_time_start_; // bgsave开始时间
    std::chrono::milliseconds bgsave_time_last_; // bgsave 耗时
    std::chrono::steady_clock::time_point bgsave_last_try_; // 最后一次执行时间
    int bgsave_last_status_; // bgsave 最后一次执行状态
    bool use_aof_checksum_;
    std::shared_ptr<replicaLink> master_; // connect to master
    std::list<replicaLink*> slaves_;

    // 本机 aof
    std::atomic_int repl_pos_; // 当前使用的index
    int repl_merge_size_; // 和 worker 数一致
    using repl_workers = std::vector<naruto::utils::Bytes>;
    std::array<std::shared_ptr<repl_workers>,2> repl_;
    std::atomic_uint64_t repl_command_incr_; // 命令自增数
    std::vector<uint8_t> back_log_;
    std::shared_ptr<sink::RotateFileStream> aof_file_;
    replConf repl_conf_;
    std::string aof_filename_before_bgsave_;
    int64_t aof_off_before_bgsave_;

    // master
    // 全局复制偏移量（一个累计值）
    long long master_repl_offset_;
    // 已经flush到aof文件的大小
    long long master_flush_repl_offset_;
    int repl_ping_slave_period_;

    // slave
    std::string master_host_;
    int master_port_;
    enum replState repl_state_;
    std::shared_ptr<ev::io> repl_ev_io_w_;
    std::shared_ptr<ev::io> repl_ev_io_r_;
    int repl_timeout_sec_;

    // RDB 文件的大小
    off_t repl_transfer_size_;
    // 已读 RDB 文件内容的字节数
    off_t repl_transfer_read_;
    // 保存 RDB 文件的临时文件的描述符
    std::shared_ptr<std::ofstream> repl_transfer_f_;
    // 保存 RDB 文件的临时文件名字
    std::string repl_transfer_tmpfile_;
    // 最近一次读入 RDB 内容的时间
    std::chrono::system_clock::time_point repl_transfer_lastio_;
    std::chrono::system_clock::time_point repl_last_interaction_;
    // 是否只读从服务器？
    bool repl_slave_ro_;
    // 连接断开的时长
    std::string repl_master_runid_;
};

extern std::shared_ptr<Replication> replptr;

}


#endif //NARUTO_REPLICATION_H
