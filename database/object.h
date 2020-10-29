//
// Created by 王振奎 on 2020/9/28.
//

#ifndef NARUTO_OBJECT_H
#define NARUTO_OBJECT_H

#include <unordered_map>
#include <glog/logging.h>

#include "utils/bytes.h"
#include "protocol/data.pb.h"

namespace naruto::database {

class object {
public:
    // 对象类型
    virtual data::TYPE type() = 0;
    // 序列化对象为 bytes 数组
    virtual void serialize(utils::Bytes &) = 0;
    virtual void deSeralize(utils::Bytes& ) = 0;
    virtual void debugString() = 0;
    virtual ~object() = default;
};

}
#endif //NARUTO_OBJECT_H
