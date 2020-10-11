//
// Created by 王振奎 on 2020/10/11.
//

#ifndef NARUTO_FACTORY_H
#define NARUTO_FACTORY_H

#include "string_.h"
#include "number.h"
#include "list_int.h"
#include "list_string.h"
#include "list_float.h"
#include "map.h"
#include "object.h"
#include "float.h"

namespace naruto::database {

class Factory {
public:
    static std::shared_ptr<object> objectBuild(int type);
};

}
#endif //NARUTO_FACTORY_H
