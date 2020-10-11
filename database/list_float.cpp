//
// Created by 王振奎 on 2020/10/6.
//

#include "list_float.h"

data::TYPE naruto::database::ListFloat::type() { return data::FLOAT_LIST; }

void naruto::database::ListFloat::get(data::VALUE &value) {
    value.set_type(type());
    for (auto v : _data){
        value.add_float_list(v);
    }
}

void naruto::database::ListFloat::set(const data::VALUE &value) {
    if (value.type() != type()) throw utils::TypeError("list data type must float");

    _data.reserve(value.float_list().size());
    for (auto v : value.number_list()){
        _data.push_back(v);
    }
}

void naruto::database::ListFloat::serialize(naruto::utils::Bytes &bytes) {

}

int naruto::database::ListFloat::len() { return _data.size(); }

void naruto::database::ListFloat::lpop(data::VALUE &value) {
    value.set_type(data::FLOAT);
    value.set_float_(_data[_data.size()]);
    _data.pop_back();
}

void naruto::database::ListFloat::ltrim(int start, int end) {
    if (_data.empty()) return;

    if (start > end || start < 0 || end < 0) throw utils::BadArgError("ltrim index end must great start");

    if (end > _data.size()) {
        if (start == end){
            start = _data.size();
        }
        end = _data.size();
    }

    auto it = _data.begin();
    for (it = it + start; it < it + end; ++it){
        _data.erase(it);
    }
}

void naruto::database::ListFloat::lpush(const data::VALUE &value) {
    if (value.type() != data::FLOAT) throw utils::BadArgError("lpush data type must float");
    _data.push_back(value.float_());
}

void naruto::database::ListFloat::lrange(int start, int end, data::VALUE &reply) {
    if (_data.empty()) return;

    if (start > end || start < 0 || end < 0) throw utils::BadArgError("lrange index end must great start");

    if (end > _data.size()) {
        if (start == end){
            start = _data.size();
        }
        end = _data.size();
    }

    reply.set_type(data::FLOAT_LIST);
    for (int i = start; i < end; ++i) {
        reply.add_float_list(_data[i]);
    }
}

void naruto::database::ListFloat::incr(int v) {
    throw utils::TypeError("list type not support incr");
}

void naruto::database::ListFloat::mapdel(const std::string &string) {
    throw utils::TypeError("list type not support mapdel");
}

void naruto::database::ListFloat::debugString() {

}
