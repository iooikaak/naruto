//
// Created by 王振奎 on 2020/9/2.
//

#ifndef NARUTO_CONNECTION_POOL_H
#define NARUTO_CONNECTION_POOL_H

#include <list>
#include <condition_variable>
#include <mutex>
#include "utils/nocopy.h"
#include "connection.h"

namespace naruto::connection{

struct ConnectionPoolOptions {
    // 最大连接数，包含使用中 和 空闲
    std::size_t max_conns = 5;

    // 最大空闲连接数
    std::size_t max_idel = 2;

    // 最大fetch等待时间，0 代表一致等待
    std::chrono::milliseconds wait_timeout{0};

    // 连接空闲时间，超过此空闲时间，再次使用时，则会关闭重连，0 代表永不空闲
    std::chrono::milliseconds connection_lifetime{5000};
};

class ConnectionPool : public utils::UnCopyable{
public:
    ConnectionPool(ConnectionPoolOptions pool_opts, ConnectOptions  opts);
    ~ConnectionPool() = default;

    std::shared_ptr<Connect> fetch();

    void release(std::shared_ptr<Connect> connect);

private:
    std::shared_ptr<Connect> _create();
    std::shared_ptr<Connect> _fetch();
    bool _need_reconnect(const std::shared_ptr<Connect>& conn) const;
    void _wait_for_connect(std::unique_lock<std::mutex>& lock);

    ConnectOptions opts_;
    ConnectionPoolOptions pool_opts_;
    std::list<std::shared_ptr<Connect>> pool_;

    std::size_t used_connections_ = 0;
    std::mutex mutex_;
    std::condition_variable cv_;
};

}



#endif //NARUTO_CONNECTION_POOL_H
