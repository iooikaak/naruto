//
// Created by 王振奎 on 2020/10/24.
//

#ifndef NARUTO_UT_CLIENT_SLAVEOF_H
#define NARUTO_UT_CLIENT_SLAVEOF_H

#include <gtest/gtest.h>

#include "utils/pack.h"
#include "protocol/replication.pb.h"
#include "connection/connection.h"
#include "connection/connection_pool.h"
#include "protocol/client.pb.h"

class TestSlaveof : public ::testing::Test{
public:
    void SetUp() { /* NOLINT */
        naruto::connection::ConnectOptions options;
        options.host = "127.0.0.1";
        options.port = 7291;
        naruto::connection::ConnectionPoolOptions pool_opts;
        pool = std::make_shared<naruto::connection::ConnectionPool>(pool_opts, options);
    }
    std::shared_ptr<naruto::connection::ConnectionPool> pool;
};

TEST_F(TestSlaveof, slaveof){ /* NOLINT */
    auto conn = pool->fetch();
    conn.connect();

    replication::command_slaveof slaveof;
    slaveof.set_ip("127.0.0.1");
    slaveof.set_port(7290);

    naruto::utils::Bytes pack;
    naruto::utils::Pack::serialize(slaveof, replication::SLAVEOF, pack);

    conn.send(pack);

    naruto::utils::Bytes packReplay;
    conn.recv(packReplay);
    client::command_reply reply;
    auto type = naruto::utils::Pack::deSerialize(packReplay, reply);
    LOG(INFO) << "recv type:" << type << " msg:" << reply.DebugString();
}

#endif //NARUTO_UT_CLIENT_SLAVEOF_H
