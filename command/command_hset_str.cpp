//
// Created by 王振奎 on 2020/10/6.
//


#include "command_hset_str.h"

void naruto::command::CommandHsetStr::exec(naruto::narutoClient *client) {
    client::command_hset_str cmd;
    auto type = utils::Pack::deSerialize(client->rbuf, cmd);

    auto element =  database::buckets->get(cmd.key(), cmd.field());

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
        database::buckets->put(cmd.key(), cmd.field(), element);
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
