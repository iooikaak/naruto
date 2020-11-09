//
// Created by 王振奎 on 2020/10/24.
//

#include "command_ping.h"

#include "client.h"
#include "protocol/replication.pb.h"

void naruto::command::CommandPing::exec(naruto::narutoClient *client) {
    LOG(INFO) << "naruto::command::CommandPing::exec...";
    replication::command_pong pong;
    pong.set_ip("127.0.0.1");
    client->sendMsg(pong, replication::PONG);
}
