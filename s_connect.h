//
// Created by 王振奎 on 2020/8/14.
//

#ifndef NARUTO_S_CONNECT_H
#define NARUTO_S_CONNECT_H

#include "net/connector.h"

namespace naruto{

class Connect : public naruto::net::Connector{
public:
    Connect(const naruto::net::ConnectOptions&, int fd);

    // 客户端创建时间
    time_t _ctime;
    // 客户端最后一次和服务器互动的时间
    time_t last_interaction;

    // 客户端标识
    int _flags;

    // 如果是从服务器，代表复制状态
    int _repl_state_of;
    int _repl_fd;
    // 读取主服务器传来的 RDB 文件的偏移量
    off_t _repl_offset;
    // 主服务器传来的 RDB 文件的大小
    off_t _repl_dbsize;

    // 主服务器复制偏移量
    long long _master_repl_offset;
    // 从服务器最后一次发送 REPLCONF ACK 时的偏移量
    long long _master_repl_ack_offset;
    // 从服务器最后一次发送 REPLCONF ACK 的时间
    long long _master_repl_ack_time;
    // 主服务器的 master run ID
    // 保存在客户端，用于执行部分重同步
    std::string _repl_run_id;
    // 从服务器的监听端口号
    int _slave_listening_port;

};

}



#endif //NARUTO_S_CONNECT_H
