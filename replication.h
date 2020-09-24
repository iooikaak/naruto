//
// Created by 王振奎 on 2020/9/18.
//

#ifndef NARUTO_REPLICATION_H
#define NARUTO_REPLICATION_H

#include <vector>
#include <atomic>
#include <ev++.h>

#include "utils/bytes.h"

// 未开始复制
#define REPL_STATE_NONE 0

// 准备和 master 建立连接
#define REPL_STATE_CONNECT 1

// 已经和 master 建立连接，未进行通信确认
#define REPL_STATE_CONNECTING 2

// 发送 Ping 到 master ，准备接受 master Pong 确认
#define REPL_STATE_RECEIVE_PONG 3

// 已经和master建立连接，并且 PING-PONG 确认连接有效
// 发送了 sync 或者 psync 请求，开始接受 同步数据
#define REPL_STATE_TRASFER 4

// 首次全量同步已经完成，和master连接正常连接，增量同步
#define REPL_STATE_CONNECTED 5

namespace naruto{

class Replication {
public:
    Replication(bool master,int back_log_size, int merge_threads_size, int repl_timeout_sec);

    void onReplCron(ev::timer&, int);

private:
    // 每个线程会持有一个 写命令的 双 buffer
    using repl_bytes = std::vector<std::shared_ptr<naruto::utils::Bytes>>;
    bool _master;

    // 消息自增id，用于多线程命名重排序
    std::atomic_uint64_t _repl_incr_id;

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
    int _repl_state;
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
    // 是否只读从服务器？
    bool _repl_slave_ro;
    // 连接断开的时长
    time_t _repl_down_since;
    std::string _repl_master_runid;
    // 初始化偏移量
    long long _repl_master_initial_offset;
};


}



#endif //NARUTO_REPLICATION_H
