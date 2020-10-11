//
// Created by 王振奎 on 2020/10/11.
//

#ifndef NARUTO_PACK_H
#define NARUTO_PACK_H

#include "bytes.h"
#include <google/protobuf/message.h>

namespace naruto::utils{

class Pack {
public:
    /*
     * 打包 @msg 到 @pack 中
     * 打包后的 pack 可直接在系统中传输并被其他机器识别
     * */
    static void serialize(utils::Bytes& pack, ::google::protobuf::Message& msg, uint64_t type);

    /*
     * 从 @pack 解析到 @msg,返回消息的类型，如果解析失败返回 -1
     * */
    static uint64_t deSerialize(utils::Bytes& pack, ::google::protobuf::Message& msg);
};

}



#endif //NARUTO_PACK_H
