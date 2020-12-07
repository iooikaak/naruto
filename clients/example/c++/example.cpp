//
// Created by 王振奎 on 2020/9/1.
//

#include <glog/logging.h>
#include <thread>
#include "connection/connection_pool.h"
#include "connection/connection.h"
#include "clients/c++/kunai.h"

void test01(){
    naruto::connection::ConnectOptions options;
    options.host = "127.0.0.1";
//    options.port = 7292;
    naruto::connection::ConnectionPoolOptions pool_opts;
    naruto::kunai::Options opts;
    opts.poolops = pool_opts;
    opts.connops = options;
    naruto::kunai::Kunai c(opts);
//    for (int i = 0; i < 100000; ++i) {
        auto reply = c.hget("test_0",{"field_0"});
        LOG(INFO) << "reply:" << "\n" << reply.DebugString();
//        std::this_thread::sleep_for(std::chrono::milliseconds(100));
//    }
}

void test02(){
    naruto::connection::ConnectOptions options;
    options.host = "127.0.0.1";
    options.port = 7293;
    naruto::connection::ConnectionPoolOptions pool_opts;
    naruto::kunai::Options opts;
    opts.poolops = pool_opts;
    opts.connops = options;
    naruto::kunai::Kunai c(opts);
    auto reply = c.slaveof("127.0.0.1",7291);
    LOG(INFO) <<"reply:" << reply.DebugString();
//    std::this_thread::sleep_for(std::chrono::seconds(5));
//
//    auto reply1 = c.hget("test_1",{"field_1"});
//    LOG(INFO) << "reply:" << "\n" << reply1.DebugString();
}

void test03(){
    naruto::connection::ConnectOptions options;
    options.host = "127.0.0.1";
    naruto::connection::ConnectionPoolOptions pool_opts;
    naruto::kunai::Options opts;
    opts.poolops = pool_opts;
    opts.connops = options;
    naruto::kunai::Kunai c(opts);
    int64_t count = 1;
    for (int64_t i = 0; i < count; ++i) {
//        c.hset("test_" + std::to_string(i), "field_" + std::to_string(i), i+5);
        c.hset("test_" + std::to_string(i), "field_" + std::to_string(i), "xxx_" +std::to_string(i));
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main(int argc, char* argv[]){
//    test01();
//    test02();
//    test02();
    test03();
}