//
// Created by 王振奎 on 2020/11/1.
//

#include "command_psync.h"
#include "client.h"
#include "protocol/replication.pb.h"
#include "utils/pack.h"
#include "replication.h"
#include "connect_worker.h"
#include "naruto.h"

void naruto::command::CommandPsync::exec(naruto::narutoClient *client) {
    replication::command_psync psync;
    utils::Pack::deSerialize(client->rbuf, psync);
    auto worker = &workers[client->worker_id];
    auto server = worker->server;

    
}
