//
// Created by 王振奎 on 2020/10/3.
//

#include "command_hget.h"

void naruto::command::CommandHget::call(std::shared_ptr<database::Buckets> data, const naruto::utils::Bytes &request,
        naruto::utils::Bytes &response) {

    uint32_t len = request.getInt(0);
    client::command_hget cmd;
    cmd.ParseFromArray(&request.data()[PACK_HEAD_LEN],len - PACK_HEAD_LEN);
    client::command_hget_reply reply;
    auto state = reply.mutable_state();
    auto element =  data->get(cmd.key(), cmd.field());

    if (element){ // 拿到了元素
        state->set_errcode(0);
        element->ptr->get(*reply.mutable_data());
    }else{
        state->set_errcode(1);
    }
//    LOG(INFO) << "command hget reply:" << reply.DebugString();
    response.putMessage(reply, COMMAND_CLIENT_HGET);
}
