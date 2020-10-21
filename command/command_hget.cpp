//
// Created by 王振奎 on 2020/10/3.
//

#include <client.h>
#include "command_hget.h"
#include "utils/pack.h"
#include "connect_worker.h"

void naruto::command::CommandHget::exec(naruto::narutoClient *client) {
    client::command_hget cmd;
    utils::Pack::deSerialize(client->rbuf, cmd);

    client::command_hget_reply reply;
    auto state = reply.mutable_state();
    auto element = workers[client->worker_id].buckets->get(cmd.key(), cmd.field());

    if (element){ // 拿到了元素
        state->set_errcode(0);
        element->ptr->get(*reply.mutable_data());
    }else{
        state->set_errcode(1);
    }
    client->sendMsg(reply, client::HGET);
}
