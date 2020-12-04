//
// Created by 王振奎 on 2020/10/3.
//

#include "command_nf.h"

#include "client.h"
#include "protocol/client.pb.h"
#include "protocol/command_types.pb.h"

void naruto::command::CommandNF::exec(naruto::narutoClient *client) {
    client::command_reply reply;
    reply.set_errcode(1);
    reply.set_errmsg("Command Not Found");
    client->sendMsg(reply, client::Type::NF);
}