//
// Created by 王振奎 on 2020/10/6.
//


#include <client.h>
#include "command_hset.h"
#include "database/factory.h"
#include "utils/pack.h"
#include "connect_worker.h"

void naruto::command::CommandHset::exec(naruto::narutoClient *client) {
    client::command_hset cmd;
    utils::Pack::deSerialize(client->rbuf, cmd);

    // 构造元素
    auto element = std::make_shared<database::element>();
    element->create = std::chrono::steady_clock::now();
    element->lru =  element->create;
    if (cmd.ttl() > 0){
        element->expire = element->create + std::chrono::seconds(cmd.ttl());
    }

    auto value = cmd.value();
    element->ptr = database::Factory::objectBuild(value.type());
    element->ptr->set(value);

    // debug
    element->ptr->debugString();
    // set data
    workers[client->worker_id].buckets->put(cmd.key(), cmd.field(), element);

    client::command_reply reply;
    reply.set_errcode(1);
    client->sendMsg(reply,client::HSET);
}
