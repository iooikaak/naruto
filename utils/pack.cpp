//
// Created by 王振奎 on 2020/10/11.
//

#include "pack.h"

void naruto::utils::Pack::serialize(const ::google::protobuf::Message &msg, uint16_t type,
        naruto::utils::Bytes &pack) {
    pack.putMessage(msg, type);
}

void naruto::utils::Pack::serialize(const ::google::protobuf::Message &msg, uint16_t type, std::ostream *out) {
    uint32_t body_size = msg.ByteSizeLong();
    uint32_t pack_size = PACK_HEAD_LEN + body_size;
    uint16_t flag = 0;
    out->write((const char*)&pack_size, sizeof(pack_size));
    out->write((const char*)&flag, sizeof(flag));
    out->write((const char*)&type, sizeof(type));
    msg.SerializeToOstream(out);
}

uint16_t naruto::utils::Pack::deSerialize(naruto::utils::Bytes &pack, ::google::protobuf::Message &msg) {
    auto len = pack.getInt(PACK_HEAD_SIZE_INDEX);
    auto type =  pack.getShort(PACK_HEAD_TYPE_INDEX);
    msg.ParseFromArray(&pack.data()[PACK_HEAD_LEN], len - PACK_HEAD_LEN);
    return type;
}

uint16_t naruto::utils::Pack::deSerialize(const unsigned char *data, size_t n, google::protobuf::Message &msg) {
    auto len = (uint32_t)data[PACK_HEAD_SIZE_INDEX];
    auto type = (uint16_t)data[PACK_HEAD_TYPE_INDEX];
    msg.ParseFromArray(&data[PACK_HEAD_LEN], len - PACK_HEAD_LEN);
    return type;
}
