//
// Created by 王振奎 on 2020/10/11.
//

#ifndef NARUTO_UT_CLIENT_H
#define NARUTO_UT_CLIENT_H

#include <gtest/gtest.h>

#include "connection/connection.h"
#include "connection/connection_pool.h"
#include "protocol/message.pb.h"
#include "utils/bytes.h"
#include "utils/pack.h"
#include "command/command.h"

using namespace naruto;

class TestClient : public ::testing::Test{
public:

    virtual void SetUp() {
        connection::ConnectionPoolOptions poolOptions;
        connection::ConnectOptions options;
        options.host = "127.0.0.1";
        pool = new connection::ConnectionPool(poolOptions,options);
    }

    connection::ConnectionPool* pool;
};

TEST_F(TestClient, client){
    auto conn = pool->fetch();
    client::command_hset request;
    request.set_key("test");
    request.set_field("field1");
    auto value =  request.mutable_value();
    value->set_type(data::STRING);
    value->set_str("test-value");

     utils::Bytes pack;
    pack.putMessage(request, COMMAND_CLIENT_HSET);
    conn.connector->send(pack);

    conn.connector->recv(pack);
    client::command_reply reply;
    auto type = utils::Pack::deSerialize(pack, reply);
    LOG(INFO) << "type:" << type << " msg:" << reply.DebugString();
}


#endif //NARUTO_UT_CLIENT_H
