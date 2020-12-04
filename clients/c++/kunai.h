//
// Created by kwins on 2020/11/27.
//

#ifndef NARUTO_KUNAI_H
#define NARUTO_KUNAI_H

#include "connection/connection.h"
#include "connection/connection_pool.h"
#include "protocol/client.pb.h"

namespace naruto::kunai{

struct Options{
    connection::ConnectOptions connops;
    connection::ConnectionPoolOptions poolops;
};

class Kunai {
public:
    explicit Kunai(const Options& opts);
    client::command_hget_reply hget(const std::string& key, std::initializer_list<std::string> fileds);
    void hset(const std::string& key, const std::string& field, const std::string& v);
    void hset(const std::string& key, const std::string& field, int64_t v);
    void hset(const std::string& key, const std::string& field, float v);
    void hincr(const std::string& key, const std::string& field, float v);
    void hincr(const std::string& key, const std::string& field, int64_t v);

    client::command_reply slaveof(const std::string& ip, int port);
private:
    void execute(::google::protobuf::Message& msg, int type, ::google::protobuf::Message& reply);
    std::shared_ptr<connection::ConnectionPool> pool_;
};

}



#endif //NARUTO_KUNAI_H
