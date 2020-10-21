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
#include "command/command.h"
#include "utils/pack.h"

void test_thread_request(){
    naruto::connection::ConnectOptions options;
    options.host = "127.0.0.1";
    naruto::connection::ConnectionPoolOptions pool_opts;

    naruto::connection::ConnectionPool pool(pool_opts, options);

    std::string line;
    std::thread ts[5];
    for (int i = 0; i < 5; ++i) {
        ts[i] = std::thread([&pool]{
            while (true){
                naruto::utils::Bytes pack;

                client::command_hmget message;
                message.set_key("10000");
                message.add_fields("test");
                message.add_fields("test1");

                LOG(INFO) << "message:" << message.DebugString();
                pack.putMessage(message, client::HGET);
                auto conn =  pool.fetch();
                conn.send(pack);
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
int main(int argc, char* argv[]){
    naruto::connection::ConnectOptions options;
    options.host = "127.0.0.1";
    naruto::connection::ConnectionPoolOptions pool_opts;

    naruto::connection::ConnectionPool pool(pool_opts, options);
    auto conn = pool.fetch();

    LOG(INFO) << "............client set start.........";
    client::command_hset request;
    request.set_key("test");
    request.set_field("field1");
    auto value =  request.mutable_value();
    value->set_type(data::STRING);
    value->set_str("test-value");

    naruto::utils::Bytes pack;
    pack.putMessage(request, client::HSET);
    conn.send(pack);

    conn.recv(pack);
    client::command_reply reply;
    auto type = naruto::utils::Pack::deSerialize(pack, reply);
    LOG(INFO) << "type:" << type << " msg:" << reply.DebugString();
    LOG(INFO) << "............client set end .........";

    LOG(INFO) << "**********************************************************";

    // ============ get ============
//    LOG(INFO) << "............client get start .........";
//    client::command_hget request1;
//    request1.set_key("test");
//    request1.set_field("field1");
//
//    naruto::utils::Bytes pack1;
//    pack1.putMessage(request1, client::HGET);
//    conn.send(pack1);
//
//    conn.recv(pack1);
//    client::command_hget_reply reply1;
//    auto type1 = naruto::utils::Pack::deSerialize(pack1, reply1);
//    LOG(INFO) << "type1:" << type1 << " msg:" << reply1.DebugString();
//    LOG(INFO) << "............client get end.........";
}