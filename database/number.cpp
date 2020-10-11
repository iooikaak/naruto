//
// Created by 王振奎 on 2020/10/6.
//

#include "number.h"

data::TYPE naruto::database::Number::type() { return data::NUMBER; }

void naruto::database::Number::get(data::VALUE &value) {
    value.set_type(type());
    value.set_number(_data);
}

void naruto::database::Number::set(const data::VALUE &value) {
    if (value.type() != type()) throw utils::TypeError("data type must number");
    _data = value.number();
}

void naruto::database::Number::serialize(naruto::utils::Bytes &bytes) {

}

int naruto::database::Number::len() {
    throw utils::TypeError("number type not support len");
}

void naruto::database::Number::lpop(data::VALUE &value) {
    throw utils::TypeError("number type not support lpop");
}

void naruto::database::Number::ltrim(int start, int end) {
    throw utils::TypeError("number type not support ltrim");
}

void naruto::database::Number::lpush(const data::VALUE &value) {
    throw utils::TypeError("number type not support lpush");
}

void naruto::database::Number::lrange(int start, int end, data::VALUE &reply) {
    throw utils::TypeError("number type not support lrange");
}

void naruto::database::Number::incr(int v) { _data += v;}

void naruto::database::Number::mapdel(const std::string &string) {
    throw utils::TypeError("number type not support mapdel");
}

void naruto::database::Number::debugString() {

}
