//
// Created by 王振奎 on 2020/10/11.
//

#ifndef NARUTO_FACTORY_H
#define NARUTO_FACTORY_H

#include "object.h"

namespace naruto::database {

class Factory {
public:
    template<typename T, data::TYPE DT>
    static std::shared_ptr<T> build();
};

}
#endif //NARUTO_FACTORY_H
