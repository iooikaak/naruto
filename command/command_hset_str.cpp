//
// Created by 王振奎 on 2020/10/6.
//


#include "command_hset_str.h"
#include "client.h"
#include "database/string_.h"
#include "utils/pack.h"
#include "connect_worker.h"
#include "protocol/client.pb.h"

void naruto::command::CommandHsetStr::exec(naruto::narutoClient *client) {
    client::command_hset_str cmd;
    auto type = utils::Pack::deSerialize(client->rbuf, cmd);

    auto buckets = workers[client->worker_id].buckets;
    auto element = buckets->get(cmd.key(), cmd.field());

    std::shared_ptr<database::String> ptr;
    if (!element){
        element = std::make_shared<database::element>();
        element->create = std::chrono::steady_clock::now();
        element->lru =  element->create;
        if (cmd.ttl() > 0){
            element->expire = element->create + std::chrono::seconds(cmd.ttl());
        }

        ptr = std::make_shared<database::String>();
        element->ptr=ptr;
        buckets->put(cmd.key(), cmd.field(), element);
    } else {
        ptr = element->cast<database::String>();
    }

    ptr->set(cmd.value());

    // debug
    element->ptr->debugString();

    client::command_reply reply;
    reply.set_errcode(0);
    client->sendMsg(reply, type);
}
