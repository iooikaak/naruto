//
// Created by 王振奎 on 2020/10/24.
//

#include "command_ping.h"

#include "link/client_link.h"
#include "protocol/replication.pb.h"
#include "protocol/command_types.pb.h"

void naruto::command::CommandPing::exec(void *link) {
    auto client = static_cast<link::clientLink*>(link);
    LOG(INFO) << "Recv ping from " << client->connect->remoteAddr();
    replication::command_pong pong;
    pong.set_ip("127.0.0.1");
    client->lastinteraction = std::chrono::steady_clock::now();
    client->sendMsg(pong, cmdtype::Type::REPL_PONG);
}
