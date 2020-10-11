//
// Created by 王振奎 on 2020/10/6.
//


#include "command_hset.h"
#include "database/factory.h"

void naruto::command::CommandHset::call(std::shared_ptr<database::Buckets> data, const naruto::utils::Bytes &request,
                                        naruto::utils::Bytes &response) {
    client::command_hset cmd;
    uint32_t len = request.getInt(0);
    cmd.ParseFromArray(&request.data()[PACK_HEAD_LEN],len - PACK_HEAD_LEN);
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

    data->put(cmd.key(), cmd.field(), element);
    client::command_reply reply;
    reply.set_errcode(1);
    response.putMessage(reply, COMMAND_CLIENT_HSET);
}
