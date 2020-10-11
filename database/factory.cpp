//
// Created by 王振奎 on 2020/10/11.
//

#include "factory.h"

std::shared_ptr<naruto::database::object> naruto::database::Factory::objectBuild(int type) {
    std::shared_ptr<object> ptr = nullptr;
    switch(type){
        case data::STRING:
            ptr = std::make_shared<database::String>();
            break;
        case data::NUMBER:
            ptr = std::make_shared<database::Number>();
            break;
        case data::FLOAT:
            ptr = std::make_shared<database::Float>();
            break;
        case data::STRING_LIST:
            ptr = std::make_shared<database::ListString>();
            break;
        case data::NUMBER_LIST:
            ptr = std::make_shared<database::ListINT>();
            break;
        case data::FLOAT_LIST:
            ptr = std::make_shared<database::ListFloat>();
            break;
        case data::MAP:
            ptr = std::make_shared<database::Map>();
            break;
        default:
            break;
    }
    return ptr;
}
