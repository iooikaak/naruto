//
// Created by 王振奎 on 2020/8/14.
//

#ifndef NARUTO_NET_CONNECTOR_H
#define NARUTO_NET_CONNECTOR_H

#include <string>
#include <chrono>
#include <sys/socket.h>
#include <vector>
#include <arpa/inet.h>

#include "utils/bytes.h"
#include <google/protobuf/message.h>
#include <utils/nocopy.h>

// 定义 FD 无效值
#define FD_INVALID -1

// 定义成员函数返回值
#define CONNECT_RT_OK 0
#define CONNECT_RT_ERR -1

// 定义连接类型
#define CONNECT_FLAGS_BLOCK 0x1  // 阻塞
#define CONNECT_FLAGS_CONNECTED 0x2 // 已连接

#define CONNECT_RETRIES 10
#define CONNECT_READ_BUF_SIZE (1024*16)


/*******************************************************
 * 数据包
--------------------------------------------------------
 |    4byte      | 1byte |  1byte | 2byte |   data     |
--------------------------------------------------------
   包长度          version   flag   消息类型
   包含head
 *******************************************************/

namespace naruto{
namespace net{

class ConnectOptions{
public:
    ConnectOptions() = default;

    ConnectOptions(const ConnectOptions&) = default;
    ConnectOptions& operator=(const ConnectOptions&) = default;

    ConnectOptions(ConnectOptions&&) = default;
    ConnectOptions& operator=(ConnectOptions&&) = default;

    ~ConnectOptions() = default;

    std::string host;
    int port = 7290;

    std::chrono::milliseconds connect_timeout{ 100 };
    std::chrono::milliseconds read_timeout{ 200 };
    std::chrono::milliseconds write_timeout{ 200 };
};

class Connector {
public:
    explicit Connector(ConnectOptions );
    explicit Connector(ConnectOptions , int fd);
    explicit Connector(ConnectOptions&&);
    explicit Connector(ConnectOptions&&, int fd);

    ~Connector();

    // reset _err 使其可以重用
    void reset() noexcept;

    void send(naruto::utils::Bytes&);
    void recv(naruto::utils::Bytes&);

    void closeConnect();

    bool broken() const noexcept;

public:

    void _set_error(int type, const std::string& prefix);
    int _write_pack(naruto::utils::Bytes&);
    int _read_pack(naruto::utils::Bytes&);
    std::string _remote_addr();

    // member variable
    int _err;
    std::string _errmsg;
    int _fd;
    int _flags;
    struct sockaddr* _addr;
    size_t _addrlen;

    // buffer
    char _ibuf[CONNECT_READ_BUF_SIZE];

    ConnectOptions _options;
};

}
}
#endif //NARUTO_NET_CONNECTOR_H
