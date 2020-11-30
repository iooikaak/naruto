//
// Created by kwins on 2020/11/26.
//

#include "command_multi.h"
#include "client.h"
#include "protocol/client.pb.h"
#include "replication.h"
#include "commands.h"

void naruto::command::CommandMulti::exec(naruto::narutoClient *client) {
    client::command_multi multi;
    utils::Pack::deSerialize(client->rbuf, multi);
    for (int i = 0; i < multi.commands_size(); ++i) {
        auto type = multi.types(i);
        auto msg = (const unsigned char*)multi.commands(i).c_str();
        auto n = multi.commands(i).size();
        auto cmd = commands->fetch(type);
        if (cmd)
            cmd->execMsg(0, type, msg, n);
    }

    auto conf = const_cast<replConf&>(replica->getReplConf());
    conf.db_dump_aof_name = multi.aof_name();
    conf.db_dump_aof_off = multi.aof_off();
}

void naruto::command::CommandMulti::execMsg(uint16_t flag, uint16_t type, const unsigned char *msg, size_t n) {

}
