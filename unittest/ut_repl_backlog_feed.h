//
// Created by kwins on 2020/11/9.
//

#ifndef NARUTO_UT_REPL_BACKLOG_FEED_H
#define NARUTO_UT_REPL_BACKLOG_FEED_H

#include <gtest/gtest.h>
#include "replication.h"
#include "protocol/client.pb.h"
#include <thread>
class TestReplBacklogFeed : public ::testing::Test{
public:
    void SetUp(){

    }
    naruto::Replication repl_{10};
};

TEST_F(TestReplBacklogFeed, backlog){
    for (int i = 0; i < 10; ++i) {
        for (int j = 0; j < 10000; ++j) {
            client::command_hget_int cmd;
            cmd.set_key("test");
            cmd.set_field("haha:" + std::to_string(i) + std::to_string(j));
            repl_.backlogFeed(i, cmd, client::HGET_INT);
        }
    }
    ev_loop(ev::get_default_loop(),0);
}
#endif //NARUTO_UT_REPL_BACKLOG_FEED_H
