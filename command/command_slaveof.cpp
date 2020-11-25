//
// Created by 王振奎 on 2020/10/15.
//

#include "command_slaveof.h"

#include "client.h"
#include "utils/pack.h"
#include "replication.h"
#include "protocol/client.pb.h"
#include "protocol/replication.pb.h"

void naruto::command::CommandSlaveof::exec(naruto::narutoClient *client) {
    replication::command_slaveof cmd;
    auto type = utils::Pack::deSerialize(client->rbuf, cmd);

    if (type != replication::SLAVEOF) return;

    replica->setReplState(replState::CONNECT);
    replica->setMasterPort(cmd.port());
    replica->setMasterHost(cmd.ip());
    replica->setIsMaster(false);

    client->flag = narutoClient::flags::SLAVE;

    client::CommandReply reply;
    reply.set_errcode(0);
    reply.set_errmsg("success");
    client->sendMsg(reply, replication::SLAVEOF);
}

void naruto::command::CommandSlaveof::execMsg(uint16_t flag, uint16_t type, const unsigned char *msg, size_t n) {

}
