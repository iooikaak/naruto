//
// Created by 王振奎 on 2020/10/3.
//

#include "list_int.h"

data::TYPE naruto::database::ListINT::type() { return data::NUMBER; }

void naruto::database::ListINT::get(data::VALUE &value) {
    value.set_type(type());
    for (long long v : _data) {
        value.add_number_list(v);
    }
}

void naruto::database::ListINT::set(const data::VALUE &value) {
    if (value.type() != type()) throw utils::TypeError("list data type must number");

    _data.reserve(value.number_list().size());
    for (long long i : value.number_list()) {
        _data.push_back(i);
    }
}

void naruto::database::ListINT::serialize(::naruto::utils::Bytes &bytes) {

}

int naruto::database::ListINT::len() { return _data.size(); }

void naruto::database::ListINT::lpop(data::VALUE &value) {
    value.set_type(data::NUMBER);
    value.set_number(_data[_data.size()]);
    _data.pop_back();
}

void naruto::database::ListINT::ltrim(int start, int end) {
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

void naruto::database::ListINT::lpush(const data::VALUE &value) {
    if (value.type() != data::NUMBER) throw utils::BadArgError("lpush data type must number");
    _data.push_back(value.number());
}

void naruto::database::ListINT::lrange(int start, int end, data::VALUE &reply) {
    if (_data.empty()) return;

    if (start > end || start < 0 || end < 0) throw utils::BadArgError("lrange index end must great start");

    if (end > _data.size()) {
        if (start == end){
            start = _data.size();
        }
        end = _data.size();
    }

    reply.set_type(data::NUMBER_LIST);
    for (int i = start; i < end; ++i) {
        reply.add_number_list(_data[i]);
    }
}

void naruto::database::ListINT::incr(int v) {
    throw utils::TypeError("list type not support incr");
}

void naruto::database::ListINT::mapdel(const std::string &string) {
    throw utils::TypeError("list type not support mapset");
}

void naruto::database::ListINT::debugString() {

}
