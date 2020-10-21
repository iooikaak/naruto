//
// Created by 王振奎 on 2020/10/11.
//

#include "pack.h"

void naruto::utils::Pack::serialize(const ::google::protobuf::Message &msg, uint64_t type,
        naruto::utils::Bytes &pack) {
    LOG(INFO) << "Pack::serialize--->>" << pack.size();
    pack.putMessage(msg, type);
}

uint64_t naruto::utils::Pack::deSerialize(naruto::utils::Bytes &pack, ::google::protobuf::Message &msg) {
    auto len = pack.getInt(PACK_HEAD_SIZE_INDEX);
    auto type =  pack.getShort(PACK_HEAD_TYPE_INDEX);
    msg.ParseFromArray(&pack.data()[PACK_HEAD_LEN], len - PACK_HEAD_LEN);
    return type;
}
