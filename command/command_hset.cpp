//
// Created by kwins on 2020/11/23.
//

#include "command_hset.h"

#include "client.h"
#include "protocol/client.pb.h"
#include "protocol/command_types.pb.h"
#include "utils/pack.h"
#include "database/buckets.h"
#include "database/number.h"
#include "database/string_.h"

void naruto::command::CommandHset::exec(naruto::narutoClient *client) {
    LOG(INFO) << "CommandHset.....0";
    auto len =  client->rbuf.getInt();
    auto flag = client->rbuf.getShort();
    auto type = client->rbuf.getShort();

    auto reply = execMsg(flag, type, &client->rbuf.data()[PACK_HEAD_LEN], (len - PACK_HEAD_LEN));
    client->sendMsg(reply, client::HSET);
}

client::command_reply
naruto::command::CommandHset::execMsg(uint16_t flag, uint16_t type, const unsigned char* s, size_t n) {
    client::command_hset hset;
    hset.ParseFromArray(s, n);

    std::shared_ptr<database::element> data = database::buckets->get(hset.key(), hset.field());
    if (!data){
        data = std::make_shared<database::element>();
        data->create = std::chrono::system_clock::now();
        if (hset.ttl() > 0){
            data->expire = data->create + std::chrono::seconds(hset.ttl());
        }
        if (hset.value().has_bytes_v()){
            data->ptr = std::make_shared<database::String>();
        } else if (hset.value().has_float_v()){
            data->ptr = std::make_shared<database::Number<float>>();
        } else if (hset.value().has_int64_v()){
            data->ptr = std::make_shared<database::Number<int64_t>>();
        }
        database::buckets->put(hset.key(), hset.field(), data);
    }

    client::command_reply reply;
    if (hset.value().has_bytes_v()){
        auto element = data->cast<database::String>();
        if (!element){
            reply.set_errcode(1);
            reply.set_errmsg("data type not string");
        }else{
            element->set(hset.value().bytes_v().value());
        }
    } else if (hset.value().has_float_v()){
        auto element = data->cast<database::Number<float>>();
        if (!element){
            reply.set_errcode(1);
            reply.set_errmsg("data type not float");
        }else{
            element->set(hset.value().float_v().value());
        }
    } else if (hset.value().has_int64_v()){
        auto element = data->cast<database::Number<int64_t>>();
        if (!element){
            LOG(INFO) << "1";
            reply.set_errcode(1);
            reply.set_errmsg("data type not int64");
        }else{
            LOG(INFO) << "2";
            element->set(hset.value().int64_v().value());
        }
    }
    data->lru = std::chrono::system_clock::now();
    return reply;
}
