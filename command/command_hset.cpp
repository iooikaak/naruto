//
// Created by kwins on 2020/11/23.
//

#include "command_hset.h"

#include "client.h"
#include "protocol/client.pb.h"
#include "utils/pack.h"
#include "database/buckets.h"
#include "database/number.h"
#include "database/string_.h"

void naruto::command::CommandHset::exec(naruto::narutoClient *client) {
    client::CommandHset hset;
    auto type = utils::Pack::deSerialize(client->rbuf, hset);

    for (int i = 0; i < hset.fields_size(); ++i) {
        std::shared_ptr<database::element> data = database::buckets->get(hset.key(), hset.fields(i));
        if (!data){
            data = std::make_shared<database::element>();
            data->create = std::chrono::system_clock::now();
            if (hset.ttls(i) > 0){
                data->expire = data->create + std::chrono::seconds(hset.ttls(i));
            }

            if (hset.values(i).type() == tensorflow::INT){
                data->ptr = std::make_shared<database::Number<int64_t>>();

            } else if (hset.values(i).type() == tensorflow::FLOAT){
                data->ptr = std::make_shared<database::Number<float>>();

            } else if (hset.values(i).type() == tensorflow::BYTES){
                data->ptr = std::make_shared<database::String>();
            }
        }

        data->lru = std::chrono::system_clock::now();

        if (hset.values(i).type() == tensorflow::INT){
            auto element = data->cast<database::Number<int64_t>>();
            element->set(hset.values(i).int64_value().value());

        } else if (hset.values(i).type() == tensorflow::FLOAT){
            auto element = data->cast<database::Number<float>>();
            element->set(hset.values(i).float_value().value());

        } else if (hset.values(i).type() == tensorflow::BYTES){
            auto element = data->cast<database::String>();
            element->set(hset.values(i).bytes_value().value());
        }
    }

    client::CommandReply reply;
    reply.set_errcode(0);
    client->sendMsg(reply, type);
}