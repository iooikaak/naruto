//
// Created by kwins on 2020/11/30.
//

#include "command_pong.h"
#include <glog/logging.h>
#include "link//client_link.h"

void naruto::command::CommandPong::exec(void* link) {
    auto client = static_cast<link::clientLink*>(link);
    LOG(INFO) << "Recv pong from " << client->connect->remoteAddr();
    client->lastinteraction = std::chrono::steady_clock::now();
}
