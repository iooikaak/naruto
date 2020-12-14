//
// Created by kwins on 2020/12/8.
//

#ifndef NARUTO_CLIENT_LINK_H
#define NARUTO_CLIENT_LINK_H

#include <ev++.h>
#include <chrono>
#include <utils/bytes.h>

#include "connection/connection.h"

namespace naruto::link{

class clientLink {
public:
    enum class flags{
        NF = 0,
        MASTER = 1,
        SLAVE = 2,
        PFAIL = 4,
        FAIL = 8,
        MYSELF = 16,
        HANDSHAKE = 32,
        MEET = 64,
    };
public:
    explicit clientLink(connection::ConnectOptions opts, int wid);
    explicit clientLink(int wid, int sd);

    virtual void onRead(ev::io& watcher, int events);
    virtual void onWrite(ev::io& watcher, int events);

    void sendMsg(const ::google::protobuf::Message& msg, uint16_t type); // 异步
    uint16_t sendMsg(const ::google::protobuf::Message&question, uint16_t type, // 同步
                     ::google::protobuf::Message& answer) const;
    uint64_t recvMsg(::google::protobuf::Message& msg);
    uint64_t read(::google::protobuf::Message& msg) const; // 同步
    void write(const ::google::protobuf::Message& msg, uint16_t type) const; // 同步
    virtual void close() const;
    std::string remoteAddr() const;

    int worker_id;
    std::shared_ptr<ev::io> c_rio; // 读取连接上的数据,master or client
    ev::io c_wio;
    std::string remote_addr;
    std::shared_ptr<connection::Connect> connect;
    unsigned int flag;
    std::chrono::system_clock::time_point ctime;
    std::chrono::steady_clock::time_point lastinteraction;
    utils::Bytes wbuf;
    utils::Bytes rbuf;
};

}



#endif //NARUTO_CLIENT_LINK_H
