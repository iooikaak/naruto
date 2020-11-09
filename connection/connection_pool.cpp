//
// Created by 王振奎 on 2020/9/2.
//

#include "connection_pool.h"
#include "utils/errors.h"

namespace naruto::connection{

ConnectionPool::ConnectionPool(const ConnectionPoolOptions &pool_opts,
                               const ConnectOptions &opts) : _pool_opts(pool_opts), _opts(opts) {
    if (_pool_opts.max_conns == 0){
        naruto::utils::throw_err(CONNECT_ERROR_OTHER, "can not create an empty pool");
    }
}

Connect ConnectionPool::fetch() {
    LOG(INFO) << "ConnectionPool _used_connections:" << _used_connections << " _pool size:" << _pool.size();
    std::unique_lock<std::mutex> lock(_mutex);
    if (_pool.empty()){
        if (_used_connections == _pool_opts.max_conns){
            _wait_for_connect(lock);
        }else{
            try {
                auto connection = _create();
                connection.update_last_active();
                ++_used_connections;
                return connection;
            }catch (const naruto::utils::Error& e){
                throw;
            }
        }
    }

    auto connection = _fetch();
    lock.unlock();

    auto connection_lifetime = _pool_opts.connection_lifetime;
    if (_need_reconnect(connection, connection_lifetime)){
        try {
            connection.reconnect();
        }catch (const naruto::utils::Error& e){
            release(std::move(connection));
            throw;
        }
    }
    connection.update_last_active();
    return std::move(connection);
}

void ConnectionPool::release(Connect connection) {
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _pool.push_back(std::move(connection));
    }
    _cv.notify_one();
}

bool ConnectionPool::_need_reconnect(const Connect &conn, const std::chrono::milliseconds &lifetime) const {
    if (conn.broken()){ return true ;}

    if (lifetime > std::chrono::milliseconds(0)){
        auto now = std::chrono::steady_clock::now();
        if ((now - conn.last_active()) > lifetime ){
            return true;
        }
    }
    return false;
}

Connect ConnectionPool::_fetch() {
    assert(!_pool.empty());
    auto connection = std::move(_pool.front());
    _pool.pop_front();
    return connection;
}

Connect ConnectionPool::_create() {
    LOG(INFO) << "_create----1";
    auto conn =  Connect(_opts);
    LOG(INFO) << "_create----2";
    if (conn.connect() != CONNECT_RT_OK)
        naruto::utils::throw_err(conn.errcode(), conn.errmsg());
    return conn;
}

void ConnectionPool::_wait_for_connect(std::unique_lock<std::mutex>& lock) {
    LOG(INFO) << "_wait_for_connect..." << _pool_opts.wait_timeout.count();
    auto timeout = _pool_opts.wait_timeout;
    if (timeout > std::chrono::milliseconds(0)){
        if (_cv.wait_for(lock, timeout, [this]{
            return !(this->_pool.empty());
        })){
            naruto::utils::throw_err(CONNECT_ERROR_OTHER, "fetch a connection time out in " +
                                                          std::to_string(timeout.count()) + " milliseconds");
        }
    }else{
        _cv.wait(lock, [this] {
            return !(this->_pool.empty());
        });
    }
}

}