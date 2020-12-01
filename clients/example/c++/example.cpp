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
    options.port = 7292;
    naruto::connection::ConnectionPoolOptions pool_opts;
    naruto::kunai::Options opts;
    opts.poolops = pool_opts;
    opts.connops = options;
    naruto::kunai::Kunai c(opts);
//    for (int i = 0; i < 100000; ++i) {
        auto reply = c.hget("test_1",{"field_1"});
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
    std::this_thread::sleep_for(std::chrono::seconds(5));

    auto reply1 = c.hget("test_1",{"field_1"});
    LOG(INFO) << "reply:" << "\n" << reply1.DebugString();
}

int main(int argc, char* argv[]){
    test01();
//    test02();
}