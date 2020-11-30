//
// Created by 王振奎 on 2020/10/24.
//

#include "command_ping.h"

#include "client.h"
#include "protocol/replication.pb.h"
#include "protocol/command_types.pb.h"

void naruto::command::CommandPing::exec(naruto::narutoClient *client) {
    LOG(INFO) << "Recv ping from " << client->connect->remoteAddr();
    replication::command_pong pong;
    pong.set_ip("127.0.0.1");
    client->lastinteraction = std::chrono::steady_clock::now();
    client->sendMsg(pong, client::Type::PONG);
}

void naruto::command::CommandPing::execMsg(uint16_t flag, uint16_t type, const unsigned char *msg, size_t n) {

}
