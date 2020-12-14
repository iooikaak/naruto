//
// Created by 王振奎 on 2020/10/15.
//

#include "command_slaveof.h"

#include "replication/replica_link.h"
#include "utils/pack.h"
#include "replication/replication.h"
#include "protocol/client.pb.h"
#include "protocol/replication.pb.h"
#include "protocol/command_types.pb.h"

void naruto::command::CommandSlaveof::exec(void* link) {
    auto client = static_cast<replica::replicaLink*>(link);
    replication::command_slaveof cmd;
    auto type = utils::Pack::deSerialize(client->rbuf, cmd);
    LOG(INFO) << "Command slave of " << cmd.DebugString();
    if (type != cmdtype::Type::REPL_SLAVEOF) return;

    replica::replptr->setReplState(replica::replState::CONNECT);
    replica::replptr->setMasterPort(cmd.port());
    replica::replptr->setMasterHost(cmd.ip());
    replica::replptr->setIsMaster(false);

    client->lastinteraction = std::chrono::steady_clock::now();
    client::command_reply reply;
    reply.set_errcode(0);
    reply.set_errmsg("success");
    client->sendMsg(reply, cmdtype::Type::REPL_SLAVEOF);
}
