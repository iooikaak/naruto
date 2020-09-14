//
// Created by 王振奎 on 2020/9/2.
//

#ifndef NARUTO_CONNECTION_POOL_H
#define NARUTO_CONNECTION_POOL_H

#include <deque>
#include <condition_variable>
#include <mutex>

#include "connection.h"

namespace naruto{
namespace client{


struct ConnectionPoolOptions {
    // 最大连接数，包含使用中 和 空闲
    std::size_t max_conns = 1;

    // 最大空闲连接数
    std::size_t max_idel = 10;

    // 最大fetch等待时间，0 代表一致等待
    std::chrono::milliseconds wait_timeout{0};

    // 连接空闲时间，超过此空闲时间，再次使用时，则会关闭重连，0 代表永不空闲
    std::chrono::milliseconds connection_lifetime{5000};
};

class ConnectionPool : public naruto::utils::UnCopyable{
public:
    ConnectionPool() = default;
    ConnectionPool(const ConnectionPoolOptions& pool_opts, const naruto::net::ConnectOptions& opts);
    ~ConnectionPool() = default;

    Connect fetch();

    void release(Connect connect);

private:
    Connect _create();
    Connect _fetch();
    bool _need_reconnect(const Connect& conn, const std::chrono::milliseconds& lifetime) const;
    void _wait_for_connect(std::unique_lock<std::mutex>& lock);

    naruto::net::ConnectOptions _opts;
    ConnectionPoolOptions _pool_opts;
    std::deque<Connect> _pool;

    std::size_t _used_connections = 0;
    std::mutex _mutex;
    std::condition_variable _cv;
};

}
}



#endif //NARUTO_CONNECTION_POOL_H
