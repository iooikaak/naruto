//
// Created by 王振奎 on 2020/10/6.
//

#include "string_.h"

data::TYPE naruto::database::String::type() { return data::STRING; }

void naruto::database::String::get(data::VALUE &value) {
    value.set_type(type());
    value.set_str(_data);
}

void naruto::database::String::set(const data::VALUE &value) {
    _data = value.str();
}

void naruto::database::String::serialize(::naruto::utils::Bytes &bytes) {

}

int naruto::database::String::len() { return _data.size(); }

void naruto::database::String::lpop(data::VALUE &value) {
    throw utils::TypeError("string type not support lpop");
}

void naruto::database::String::ltrim(int start, int end) {
    throw utils::TypeError("string type not support ltrim");
}

void naruto::database::String::lpush(const data::VALUE &value) {
    throw utils::TypeError("string type not support lpush");
}

void naruto::database::String::lrange(int start, int end, data::VALUE &reply) {
    throw utils::TypeError("string type not support lrange");
}

void naruto::database::String::incr(int v) {
    throw utils::TypeError("string type not support incr");
}

void naruto::database::String::mapdel(const std::string &string) {
    throw utils::TypeError("string type not support mapdel");
}

void naruto::database::String::debugString() {
    LOG(INFO) << "-->>debug string data:" << _data;
}
