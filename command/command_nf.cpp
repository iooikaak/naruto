//
// Created by 王振奎 on 2020/10/3.
//

#include "command_nf.h"

#include "link//client_link.h"
#include "protocol/client.pb.h"
#include "protocol/command_types.pb.h"

void naruto::command::CommandNF::exec(void *link) {
    auto client = static_cast<link::clientLink*>(link);
    client::command_reply reply;
    reply.set_errcode(1);
    reply.set_errmsg("Command Not Found");
    client->sendMsg(reply, cmdtype::Type::NF);
}