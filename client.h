//
// Created by 王振奎 on 2020/9/27.
//

#ifndef NARUTO_CLIENT_H
#define NARUTO_CLIENT_H

#include <ev++.h>

#include "connection/connection.h"
#include "utils/errors.h"
#include "utils/bytes.h"

#define CLIENT_BUF_SIZE (1024*1024)

#define CLIENT_FLAG_SLAVE (1<<0)
#define CLIENT_FLAG_MASTER (1<<1)

namespace naruto{

class ConnectWorker;

class narutoClient {
public:
    narutoClient();

    void onRead(ev::io& watcher, int events);
    void onWrite(ev::io& watcher, int events);

    // 异步
    void sendMsg(const ::google::protobuf::Message& msg, uint16_t type);
    uint64_t recvMsg(::google::protobuf::Message& msg);

    // 同步
    void write(const ::google::protobuf::Message& msg, uint16_t type);
    uint64_t read(::google::protobuf::Message& msg);

    std::string remoteAddr();
    int worker_id;
    std::string ip;
    std::string port;
    std::shared_ptr<connection::Connect> connect;

    uint32_t flags;
    std::string repl_run_id;
    int repl_state{};
    std::chrono::steady_clock::time_point repl_ack_time;
    int repl_fd{};

    std::shared_ptr<ev::io> repl_ev_io_w;
    std::shared_ptr<ev::io> repl_ev_io_r;
//    std::shared_ptr<Replication> repl;


    // 读取主服务器传来的 RDB 文件的偏移量
    off_t repl_db_off{};
    // 主服务器传来的 RDB 文件的大小
    off_t repl_db_size{};

    // 主服务器的复制偏移量
    long long repl_off{};


    std::chrono::steady_clock::time_point ctime;
    std::chrono::steady_clock::time_point lastinteraction;

    utils::Bytes wbuf;
    utils::Bytes rbuf;
};


}



#endif //NARUTO_CLIENT_H
