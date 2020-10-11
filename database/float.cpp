//
// Created by 王振奎 on 2020/10/6.
//

#include "float.h"

data::TYPE naruto::database::Float::type() { return data::FLOAT; }

void naruto::database::Float::get(data::VALUE &value) {
    value.set_type(type());
    value.set_float_(_data);
}

void naruto::database::Float::set(const data::VALUE &value) {
    if (value.type() != type()) throw utils::TypeError("data type must float");
    _data = value.float_();
}

void naruto::database::Float::serialize(naruto::utils::Bytes &bytes) {

}

int naruto::database::Float::len() {
    throw utils::TypeError("float type not support len");
}

void naruto::database::Float::lpop(data::VALUE &value) {
    throw utils::TypeError("float type not support lpop");
}

void naruto::database::Float::ltrim(int start, int end) {
    throw utils::TypeError("float type not support ltrim");
}

void naruto::database::Float::lpush(const data::VALUE &value) {
    throw utils::TypeError("float type not support lpush");
}

void naruto::database::Float::lrange(int start, int end, data::VALUE &reply) {
    throw utils::TypeError("float type not support lrange");
}

void naruto::database::Float::incr(int v) {
    _data += (float)v;
}

void naruto::database::Float::mapdel(const std::string &string) {
    throw utils::TypeError("float type not support mapdel");
}

void naruto::database::Float::debugString() {

}
