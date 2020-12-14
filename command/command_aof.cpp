//
// Created by kwins on 2020/11/26.
//

#include "command_aof.h"
#include "replication/replica_link.h"
#include "protocol/client.pb.h"
#include "replication/replication.h"
#include "commands.h"
#include "utils/pack.h"

void naruto::command::CommandAof::exec(void *link) {
    auto client = static_cast<replica::replicaLink*>(link);
    client::command_aof commandAof;
    utils::Pack::deSerialize(client->rbuf, commandAof);
    for (int i = 0; i < commandAof.commands_size(); ++i) {
        auto type = commandAof.types(i);
        auto msg = (const unsigned char*)commandAof.commands(i).c_str();
        auto n = commandAof.commands(i).size();
        commands->fetch(type)->execMsg(0, type, msg, n);
        replica::replptr->backlogFeed(client->worker_id, msg, n, type);
    }
    client->repl_aof_file_name = commandAof.aof_name();
    client->repl_aof_off = commandAof.aof_off();
    LOG(INFO) << "CommandMulti commands size=" << commandAof.commands_size()
     << " master_aof_name=" << client->repl_aof_file_name << " master_aof_off=" << client->repl_aof_off;
}

client::command_reply naruto::command::CommandAof::execMsg(uint16_t flag, uint16_t type, const unsigned char *msg, size_t n) {
    return client::command_reply{};
}
