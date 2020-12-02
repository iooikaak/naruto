//
// Created by 王振奎 on 2020/9/27.
//

#ifndef NARUTO_CLIENT_H
#define NARUTO_CLIENT_H

#include <ev++.h>

#include "utils/errors.h"
#include "utils/bytes.h"
#include "utils/pack.h"
#include "connection/connection.h"

namespace naruto{
enum class replState;

class narutoClient {
public:
    enum class flags : unsigned{
        NONE = 0,
        SLAVE = 1 << 1,
        MASTER = 1 << 2
    };
public:
    narutoClient();

    void onRead(ev::io& watcher, int events);
    void onWrite(ev::io& watcher, int events);
    void onSendBulkToSlave(ev::io&, int);
    void onSendIncrToSlave(ev::timer&, int);

    void sendMsg(const ::google::protobuf::Message& msg, uint16_t type); // 异步
    uint16_t sendMsg(const ::google::protobuf::Message& question, uint16_t type,::google::protobuf::Message& answer) const;
    uint64_t recvMsg(::google::protobuf::Message& msg);

    uint64_t read(::google::protobuf::Message& msg) const; // 同步
    void write(const ::google::protobuf::Message& msg, uint16_t type) const; // 同步
    void close() const;
    std::string remoteAddr() const;

    int worker_id;
    std::shared_ptr<ev::io> cw_rio; // 读取连接上的数据,master or client
    std::shared_ptr<ev::io> repl_rio; // slave 接收 master 增量复制io
    std::shared_ptr<ev::timer> repl_tio; // master 定时发送增量数据

    std::string remote_addr;
    std::shared_ptr<connection::Connect> connect;
    unsigned int flag;
    ev::io wio;
    std::chrono::system_clock::time_point ctime;
    std::chrono::steady_clock::time_point lastinteraction;
    utils::Bytes wbuf;
    utils::Bytes rbuf;

    // 主服务器首次同步DB数据给从服务器时使用
    off_t repl_dboff;
    off_t repl_dbsize;
    std::shared_ptr<std::ifstream> repl_db_f;

    // 主服务器记录的从服务器同步状态
    // 从服务器状态
    replState repl_state;

    // 主服务器 run_id
    std::string repl_run_id;

    // 同步aof文件名
    std::string repl_aof_file_name;

    // 同步aof位置
    int64_t repl_aof_off;

    // 从服务器最后一次发送 REPLCONF ACK 的时间
    std::chrono::steady_clock::time_point repl_ack_time;
};


}



#endif //NARUTO_CLIENT_H
