//
// Created by 王振奎 on 2020/9/2.
//

#include "connection_pool.h"

#include <utility>
#include "utils/errors.h"

namespace naruto::connection{

ConnectionPool::ConnectionPool(ConnectionPoolOptions pool_opts,
                               ConnectOptions opts) : pool_opts_(pool_opts), opts_(std::move(opts)) {
    if (pool_opts_.max_conns == 0){
        naruto::utils::throw_err(CONNECT_ERROR_OTHER, "can not create an empty pool");
    }
}

std::shared_ptr<Connect> ConnectionPool::fetch() {
    LOG(INFO) << "ConnectionPool _used_connections:" << used_connections_ << " _pool size:" << pool_.size();
    std::unique_lock<std::mutex> lock(mutex_);
    if (pool_.empty()){
        if (used_connections_ >= pool_opts_.max_conns){
            _wait_for_connect(lock);
        }else{
            try {
                auto conn = _create();
                conn->update_last_active();
                ++used_connections_;
                return conn;
            }catch (const naruto::utils::Error& e){
                throw;
            }
        }
    }

    auto connection = _fetch();
    lock.unlock();
    if (_need_reconnect(connection)){
        connection->close();
        try {
            connection->reconnect();
        }catch (const naruto::utils::Error& e){
            LOG(ERROR) << "ConnectPool fetch bad connect and reconnect Fail "<< e.what();
            throw;
        }
    }
    connection->update_last_active();
    return connection;
}

void ConnectionPool::release(std::shared_ptr<Connect> connection) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pool_.push_back(std::move(connection));
    }
    cv_.notify_one();
    LOG(INFO) << "ConnectionPool size " << pool_.size();
}

bool ConnectionPool::_need_reconnect(const std::shared_ptr<Connect> &conn) const {
    if (conn->broken()){ return true ;}

    if (pool_opts_.connection_lifetime > std::chrono::milliseconds(0)){
        auto now = std::chrono::steady_clock::now();
        if ((now - conn->last_active()) > pool_opts_.connection_lifetime  ){
            return true;
        }
    }
    return false;
}

std::shared_ptr<Connect> ConnectionPool::_fetch() {
    assert(!pool_.empty());
    auto c = pool_.front();
    pool_.pop_front();
    return c;
}

std::shared_ptr<Connect> ConnectionPool::_create() {
    auto conn = std::make_shared<Connect>(opts_);
    if (conn->connect() != CONNECT_RT_OK)
        naruto::utils::throw_err(conn->errcode(), conn->errmsg());
    return conn;
}

void ConnectionPool::_wait_for_connect(std::unique_lock<std::mutex>& lock) {
    LOG(INFO) << "_wait_for_connect..." << pool_opts_.wait_timeout.count();
    auto timeout = pool_opts_.wait_timeout;
    if (timeout > std::chrono::milliseconds(0)){
        if (cv_.wait_for(lock, timeout, [this]{
            return !(this->pool_.empty());
        })){
            naruto::utils::throw_err(CONNECT_ERROR_OTHER, "fetch a connection time out in " +
                                                          std::to_string(timeout.count()) + " milliseconds");
        }
    }else{
        cv_.wait(lock, [this] {
            return !(this->pool_.empty());
        });
    }
}

}