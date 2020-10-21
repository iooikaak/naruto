//
// Created by 王振奎 on 2020/10/15.
//

#include "command_slaveof.h"

#include "client.h"
#include "connect_worker.h"
#include "naruto.h"
#include "protocol/replication.pb.h"
#include "utils/pack.h"

void naruto::command::CommandSlaveof::exec(naruto::narutoClient *client) {
    replication::command_slaveof cmd;
    auto type = utils::Pack::deSerialize(client->rbuf, cmd);

    if (type != replication::SLAVEOF) return;

    auto server = workers[client->worker_id].server;
    server->repl = std::make_shared<Replication>(workder_num);
    server->repl->setMasterPort(cmd.port());
    server->repl->setMasterHost(cmd.ip());
    server->repl->setIsMaster(false);
}
