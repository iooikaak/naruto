//
// Created by 王振奎 on 2020/10/3.
//

#include "command_hget_str.h"


#include "client.h"
#include "utils/pack.h"
#include "protocol/client.pb.h"
#include "database/string_.h"

void naruto::command::CommandHgetStr::exec(naruto::narutoClient *client) {
    client::command_hget_str cmd;
    utils::Pack::deSerialize(client->rbuf, cmd);

    client::command_hget_str_reply reply;
    auto state = reply.mutable_state();
    auto element = database::buckets->get(cmd.key(), cmd.field());

    if (element){ // 拿到了元素
        state->set_errcode(0);
        auto ptr = element->cast<database::String>();
        reply.set_data(ptr->get());
    }else{
        state->set_errcode(1);
    }
    client->sendMsg(reply, client::HGET_STRING);
}
