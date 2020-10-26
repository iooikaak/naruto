//
// Created by 王振奎 on 2020/10/24.
//

#include "command_ping.h"
#include "protocol/replication.pb.h"
#include "client.h"

void naruto::command::CommandPing::exec(naruto::narutoClient *client) {
    LOG(INFO) << "naruto::command::CommandPing::exec...";
    replication::command_pong pong;
    pong.set_ip("127.0.0.1");
    client->sendMsg(pong, replication::PONG);
}
