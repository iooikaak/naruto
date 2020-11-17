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

#define CLIENT_BUF_SIZE (1024*1024)

#define CLIENT_FLAG_SLAVE (1<<0)
#define CLIENT_FLAG_MASTER (1<<1)

namespace naruto{

enum class state;

class narutoClient {
public:
    narutoClient();

    void onReadEvent(ev::io& watcher, int events);
    void onWriteEvent(ev::io& watcher, int events);

    void sendMsg(const ::google::protobuf::Message& msg, uint16_t type); // 异步
    uint16_t sendMsg(const ::google::protobuf::Message& question, uint16_t type, ::google::protobuf::Message& answer);
    uint64_t recvMsg(::google::protobuf::Message& msg);
    void write(const ::google::protobuf::Message& msg, uint16_t type); // 同步
    uint64_t read(::google::protobuf::Message& msg); // 同步


    std::string remoteAddr();

    int worker_id;
    std::string ip;
    std::string port;
    std::shared_ptr<connection::Connect> connect;
    uint32_t flags;
    ev::io wio;
    std::chrono::steady_clock::time_point ctime;
    std::chrono::steady_clock::time_point lastinteraction;
    utils::Bytes wbuf;
    utils::Bytes rbuf;
    // 从服务器状态
    state repl_state;

    // 主服务器状态
    std::string repl_run_id; // 主服务器 run_id
    std::string repl_aof_file_name; // 同步aof文件名
    long long repl_aof_off; // 同步aof位置
    std::chrono::steady_clock::time_point repl_ack_time; // 从服务器最后一次发送 REPLCONF ACK 的时间
};


}



#endif //NARUTO_CLIENT_H
