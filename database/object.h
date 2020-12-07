//
// Created by 王振奎 on 2020/9/28.
//

#ifndef NARUTO_OBJECT_H
#define NARUTO_OBJECT_H

#include <unordered_map>
#include <glog/logging.h>

#include "utils/bytes.h"
#include "protocol/features.pb.h"

namespace naruto::database {

class object {
public:
    // 对象类型
    virtual tensorflow::Type type() = 0;
    virtual std::string typeName() = 0;
    virtual void serialize(tensorflow::Feature&) = 0;
    virtual void deSeralize(const tensorflow::Feature&) = 0;
    virtual void debugString() = 0;
    virtual ~object() = default;
};

}
#endif //NARUTO_OBJECT_H
