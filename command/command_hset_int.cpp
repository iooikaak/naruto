//
// Created by kwins on 2020/10/29.
//

#include "command_hset_int.h"

#include "protocol/client.pb.h"
#include "database/number.h"
#include "utils/pack.h"
#include "connect_worker.h"

void naruto::command::CommandHsetInt::exec(naruto::narutoClient *client) {
    client::command_hset_int cmd;
    auto type = utils::Pack::deSerialize(client->rbuf, cmd);

    auto buckets = workers[client->worker_id].buckets;
    auto element = buckets->get(cmd.key(), cmd.field());

    if (!element){
        element = std::make_shared<database::element>();
        element->create = std::chrono::steady_clock::now();
        element->lru =  element->create;
        if (cmd.ttl() > 0){
            element->expire = element->create + std::chrono::seconds(cmd.ttl());
        }

        auto ptr = std::make_shared<database::Number>();
        ptr->set(cmd.value());
        element->ptr=ptr;
        buckets->put(cmd.key(), cmd.field(), element);
    } else {
        auto origin = element->cast<database::Number>();
        origin->set(cmd.value());
    }

    // debug
    element->ptr->debugString();

    client::command_reply reply;
    reply.set_errcode(0);
    client->sendMsg(reply, type);
}
