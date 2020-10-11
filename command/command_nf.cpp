//
// Created by 王振奎 on 2020/10/3.
//

#include "command_nf.h"

void naruto::command::CommandNF::call(std::shared_ptr<database::Buckets> data,
        const naruto::utils::Bytes &request, naruto::utils::Bytes &response) {

    client::command_reply reply;
    reply.set_errcode(1);
    reply.set_errmsg("command not found");
    response.putMessage(reply, COMMAND_NOT_FOUND);
}
