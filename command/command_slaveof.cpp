//
// Created by 王振奎 on 2020/10/15.
//

#include "command_slaveof.h"

#include "client.h"
#include "connect_worker.h"
#include "naruto.h"
#include "protocol/replication.pb.h"
#include "protocol/message.pb.h"
#include "utils/pack.h"
#include "replication.h"

void naruto::command::CommandSlaveof::exec(naruto::narutoClient *client) {
    replication::command_slaveof cmd;
    auto type = utils::Pack::deSerialize(client->rbuf, cmd);

    if (type != replication::SLAVEOF) return;

    auto server = workers[client->worker_id].server;
    server->repl = std::make_shared<Replication>(workder_num);

    server->repl->setReplState(state::CONNECT);
    server->repl->setMasterPort(cmd.port());
    server->repl->setMasterHost(cmd.ip());
    server->repl->setIsMaster(false);

    client::command_reply reply;
    reply.set_errcode(0);
    reply.set_errmsg("success");
    client->sendMsg(reply, replication::SLAVEOF);
}
