//
// Created by 王振奎 on 2020/9/28.
//

#ifndef NARUTO_OBJECT_H
#define NARUTO_OBJECT_H

#include <unordered_map>
#include <glog/logging.h>

#include "utils/bytes.h"
#include "protocol/message.pb.h"
#include "protocol/data.pb.h"

namespace naruto::database {

class object {
public:
    // 对象类型
    virtual data::TYPE type() = 0;

    virtual void get(data::VALUE &) = 0;

    virtual void set(const data::VALUE &) = 0;

    // 序列化对象为 bytes 数组
    virtual void serialize(utils::Bytes &) = 0;

    // list 操作
    virtual int len() = 0;

    virtual void lpop(data::VALUE &) = 0;

    virtual void ltrim(int start, int end) = 0;

    virtual void lpush(const data::VALUE &) = 0;

    virtual void lrange(int start, int end, data::VALUE &reply) = 0;

    virtual void incr(int v) = 0;

    // map
    virtual void mapdel(const std::string &) = 0;

    virtual void debugString() = 0;

    virtual ~object() = default;

};

//object::~object() {}

}
#endif //NARUTO_OBJECT_H
