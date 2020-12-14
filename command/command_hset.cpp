//
// Created by kwins on 2020/11/23.
//

#include "command_hset.h"

#include "link/client_link.h"
#include "protocol/client.pb.h"
#include "protocol/command_types.pb.h"
#include "utils/pack.h"
#include "database/buckets.h"
#include "database/number.h"
#include "database/string_.h"
#include "replication/replication.h"

void naruto::command::CommandHset::exec(void *link) {
    auto client = static_cast<link::clientLink*>(link);
    LOG(INFO) << "CommandHset.....0";
    auto len =  client->rbuf.getInt();
    auto flag = client->rbuf.getShort();
    auto type = client->rbuf.getShort();

    auto reply = execMsg(flag, type, &client->rbuf.data()[PACK_HEAD_LEN], (len - PACK_HEAD_LEN));
    if (reply.errcode() == 0){
        replica::replptr->backlogFeed(client->worker_id, client->rbuf);
    }
    client->sendMsg(reply, cmdtype::CLIENT_HSET);
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
    reply.set_errcode(0);
    if (hset.value().has_bytes_v() &&data->ptr->type() == tensorflow::Type::BYTES){
        auto element = data->cast<database::String>();
        element->set(hset.value().bytes_v().value());

    } else if (hset.value().has_float_v() && data->ptr->type() == tensorflow::Type::FLOAT){
        auto element = data->cast<database::Number<float>>();
        element->set(hset.value().float_v().value());

    } else if (hset.value().has_int64_v() && data->ptr->type() == tensorflow::Type::INT){
        auto element = data->cast<database::Number<int64_t>>();
        element->set(hset.value().int64_v().value());
    }else{
        reply.set_errcode(1);
        reply.set_errmsg("data type is " + data->ptr->typeName());
    }
    data->lru = std::chrono::system_clock::now();
    return reply;
}
