//
// Created by 王振奎 on 2020/10/11.
//

#ifndef NARUTO_PACK_H
#define NARUTO_PACK_H

#include <ostream>
#include <google/protobuf/message.h>

#include "bytes.h"

namespace naruto::utils{

/*******************************************************
 * 数据包 和 aof 结构
---------------------------------------------------------
 |    4byte      |   2byte   |   2byte   |   data     |
---------------------------------------------------------
   包长度(包含head)    flag      data type      数据
 *******************************************************/

class Pack {
public:
    /*
     * 打包 @msg 到 @pack 中
     * 打包后的 pack 可直接在系统中传输并被其他机器识别
     * */
    static void serialize( const ::google::protobuf::Message& msg, uint16_t type, utils::Bytes& pack);

    static void serialize(const ::google::protobuf::Message& msg, uint16_t type, std::ostream* out);
    /*
     * 从 @pack 解析到 @msg,返回消息的类型，如果解析失败返回 -1
     * */
    static uint64_t deSerialize(utils::Bytes& pack, ::google::protobuf::Message& msg);


};

}



#endif //NARUTO_PACK_H
