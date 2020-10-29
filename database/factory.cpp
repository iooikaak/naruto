//
// Created by 王振奎 on 2020/10/11.
//

#include "factory.h"

template<typename T, data::TYPE DT>
std::shared_ptr<T> naruto::database::Factory::build() {
    return std::make_shared<T,DT>();
}


