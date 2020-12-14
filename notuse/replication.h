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
#include <nlohmann/json.hpp>
#include <fstream>

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
using namespace std::chrono;

struct saveparam{
    // 多少秒之内
    std::chrono::milliseconds ms;

    // 发生多少次修改
    int changes;
};

enum class replState{
    NONE = 0,          // 未开始复制
    CONNECT,           // 准备和 master 建立连接
    CONNECTING,        // 已经和 master 建立连接，未进行通信确认
    RECEIVE_PONG,      // 发送 Ping 到 master ，准备接受 master Pong 确认
    WAIT_BGSAVE,       // 等待bgsave完成
    TRANSFOR,          // 已经和master建立连接，并且 PING-PONG 确认连接有效,开始全量同步
    CONNECTED,         // 首次全量同步已经完成，和master连接正常连接，开始正常增量同步，一开始会有增量延迟情况
};

struct replConf {
    bool is_master = false;
    std::string run_id;             // 服务器id
    std::string db_dump_aof_name;   // DB文件 Dump 时的aof文件名
    int64_t db_dump_aof_off = 0;    // DB文件 Dump 时aof文件位置
    std::string master_ip;
    int master_port = 0;
    std::string master_run_id;      // 如果是slave则代表master id
    std::string master_aof_name;    // 如果是slave则代表已同步完成的aof文件名
    int64_t master_aof_off = 0;     // 如果是slave则代表已同步完成的aof文件位置

    void to_json(nlohmann::json& j){
        j["master"] = is_master;
        j["run_id"] = run_id;
        j["db_dump_aof_name"] = db_dump_aof_name;
        j["db_dump_aof_off"] = db_dump_aof_off;
        j["master_ip"] = master_ip;
        j["master_port"] = master_port;
        j["master_run_id"] = master_run_id;
        j["master_aof_name"] = master_aof_name;
        j["master_aof_off"] = master_aof_off;
    }

    void toJson(const std::string& filename){
        std::string tmpfile = filename + ".tmp";
        std::ofstream out(tmpfile, std::ios::out| std::ios::trunc);
        if (!out.is_open()) {
            LOG(ERROR) << "Replica conf toJson " << strerror(errno);
            return;
        }
        nlohmann::json j;
        to_json(j);
        out << j.dump(4);
        out.close();
        if (::rename(tmpfile.c_str(), filename.c_str()) == -1){
            LOG(INFO) << "Repl conf to json " << strerror(errno);
        }
    }

    void from_json(const nlohmann::json& j){
        j.at("master").get_to(is_master);
        j.at("run_id").get_to(run_id);
        j.at("db_dump_aof_name").get_to(db_dump_aof_name);
        j.at("db_dump_aof_off").get_to(db_dump_aof_off);
        j.at("master_run_id").get_to(master_run_id);
        j.at("master_ip").get_to(master_ip);
        j.at("master_port").get_to(master_port);
        j.at("master_aof_name").get_to(master_aof_name);
        j.at("master_aof_off").get_to(master_aof_off);
    }

    void fromJson(const std::string& filename){
        std::ifstream in(filename, std::ios::in);
        if (!in.is_open()) {
            LOG(WARNING) << "Replica parse conf file " << strerror(errno);
            return;
        }
        nlohmann::json j;
        in >> j;
        from_json(j);
        in.close();
    }

    friend std::ostream& operator<<(std::ostream& out, const replConf& repl_conf){
        out << "master:" << repl_conf.is_master << "\n"
            << "run_id:" << repl_conf.run_id << "\n"
            << "db_dump_aof_name:" << repl_conf.db_dump_aof_name << "\n"
            << "db_dump_aof_off:" << repl_conf.db_dump_aof_off << "\n"
            << "master_ip:" << repl_conf.master_ip << "\n"
            << "master_port:" << repl_conf.master_port << "\n"
            << "master_run_id:" << repl_conf.master_run_id << "\n"
            << "master_aof_name:" << repl_conf.master_aof_name << "\n"
            << "master_aof_off:" << repl_conf.master_aof_off << "\n"
            ;
        return out;
    }
};

class Replication{
public:
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
    void addSlave(narutoClient*);

    void cacheMaster();
    void discardCacheMaster();
    void statSlave(); // close slave 时记录一些统计数据
    void stop();
    ~Replication();

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
    std::list<saveparam> saveparams_;

    // bgsave dump db
    int bgsave_child_pid_; // bgsave 子进程 pid
    std::chrono::milliseconds bgsave_fork_spends_; // bgsave fork 耗时
    std::chrono::steady_clock::time_point bgsave_time_start_; // bgsave开始时间
    std::chrono::milliseconds bgsave_time_last_; // bgsave 耗时
    std::chrono::steady_clock::time_point bgsave_last_try_; // 最后一次执行时间
    int bgsave_last_status_; // bgsave 最后一次执行状态
    bool use_aof_checksum_;
    std::shared_ptr<narutoClient> master_; // connect to master
    std::list<narutoClient*> slaves_;

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

extern std::shared_ptr<Replication> replica;

}



#endif //NARUTO_REPLICATION_H
