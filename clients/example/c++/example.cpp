//
// Created by 王振奎 on 2020/9/1.
//

#include <glog/logging.h>
#include "connection/connection_pool.h"
#include "connection/connection.h"
#include "clients/c++/kunai.h"

void test01(){
    naruto::connection::ConnectOptions options;
    options.host = "127.0.0.1";
    naruto::connection::ConnectionPoolOptions pool_opts;
    naruto::kunai::Options opts;
    opts.poolops = pool_opts;
    opts.connops = options;
    naruto::kunai::Kunai c(opts);

    auto reply = c.hget("test_1",{"field_1"});
    LOG(INFO) << "reply:" << "\n" << reply.DebugString();
}

void test02(){
    naruto::connection::ConnectOptions options;
    options.host = "127.0.0.1";
    options.port = 7291;
    naruto::connection::ConnectionPoolOptions pool_opts;
    naruto::kunai::Options opts;
    opts.poolops = pool_opts;
    opts.connops = options;
    naruto::kunai::Kunai c(opts);
    c.slaveof("127.0.0.1",7290);
}

int main(int argc, char* argv[]){
//    test01();
    test02();
}