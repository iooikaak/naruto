//
// Created by kwins on 2020/10/29.
//

#include "command_hget_int.h"

#include "client.h"
#include "utils/pack.h"
#include "protocol/client.pb.h"
#include "database/number.h"

void naruto::command::CommandHgetInt::exec(naruto::narutoClient *client) {
    client::command_hget_int cmd;
    auto type = utils::Pack::deSerialize(client->rbuf, cmd);

    client::command_hget_int_reply reply;
    auto state = reply.mutable_state();
    auto element = database::buckets->get(cmd.key(), cmd.field());

    if (element){ // 拿到了元素
        state->set_errcode(0);
        auto ptr = element->cast<database::Number>();
        reply.set_data(ptr->get());
    }else{
        state->set_errcode(1);
    }
    client->sendMsg(reply, type);
}