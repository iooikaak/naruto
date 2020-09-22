//
// Created by 王振奎 on 2020/9/1.
//
#include <string>
#include <thread>
#include <chrono>
#include <glog/logging.h>

#include "connection/connection_pool.h"
#include "connection/connection.h"
#include "protocol/message.pb.h"
#include "protocol/message_type.h"


int main(int argc, char* argv[]){
    naruto::net::ConnectOptions options;
    options.host = "127.0.0.1";
    naruto::connection::ConnectionPoolOptions pool_opts;

    naruto::connection::ConnectionPool pool(pool_opts, options);

    std::string line;
    std::thread ts[5];
    for (int i = 0; i < 5; ++i) {
        ts[i] = std::thread([&pool]{
            while (true){
//        line.clear();
//        LOG(INFO) << "Enter string to send:";
//        getline(std::cin, line);
//
//        LOG(INFO) << "Enter:" << line;
//        if (line == "quit") break;

                naruto::utils::Bytes pack;

                protocol::command_hmget message;
                message.set_key("10000");
                message.add_fields("test");
                message.add_fields("test1");

                LOG(INFO) << "message:" << message.DebugString();
                pack.putMessage(message, COMMAND_HMGET);
                auto conn =  pool.fetch();
                conn.connector->send(pack);
                pool.release(std::move(conn));

                LOG(INFO) << "Send  success, len:" << pack.size();
                std::this_thread::sleep_for(std::chrono::milliseconds (100));
            }
        });
    }

    for (int j = 0; j < 5; ++j) {
        ts[j].join();
    }
}