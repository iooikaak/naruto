//
// Created by kwins on 2020/11/30.
//

#include "command_pong.h"
#include <glog/logging.h>
#include "client.h"

void naruto::command::CommandPong::exec(naruto::narutoClient *client) {
    LOG(INFO) << "Recv pong from " << client->connect->remoteAddr();
    client->lastinteraction = std::chrono::steady_clock::now();
}

void naruto::command::CommandPong::execMsg(uint16_t flag, uint16_t type, const unsigned char *msg, size_t n) {

}
