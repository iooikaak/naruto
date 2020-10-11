//
// Created by 王振奎 on 2020/10/6.
//

#include "list_string.h"

data::TYPE naruto::database::ListString::type() { return data::STRING_LIST;}

void naruto::database::ListString::get(data::VALUE &value) {
    value.set_type(type());
    for (auto v : _data) {
        value.add_str_list(std::move(v));
    }
}

void naruto::database::ListString::set(const data::VALUE &value) {
    if (value.type() != type()) throw utils::TypeError("list data type must str");

    _data.reserve(value.str_list().size());
    for (std::string v : value.str_list()) {
        _data.emplace_back(std::move(v));
    }
}

void naruto::database::ListString::serialize(naruto::utils::Bytes &bytes) {

}

int naruto::database::ListString::len() { return _data.size(); }

void naruto::database::ListString::lpop(data::VALUE &value) {
    value.set_type(data::STRING);
    value.set_str(_data[_data.size()]);
    _data.pop_back();
}

void naruto::database::ListString::ltrim(int start, int end) {
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

void naruto::database::ListString::lpush(const data::VALUE &value) {
    if (value.type() != data::STRING) throw utils::BadArgError("lpush data type must str");
    _data.push_back(value.str());
}

void naruto::database::ListString::lrange(int start, int end, data::VALUE &reply) {
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
        reply.add_str_list(_data[i]);
    }
}

void naruto::database::ListString::incr(int v) {
    throw utils::TypeError("list type not support incr");
}

void naruto::database::ListString::mapdel(const std::string &string) {
    throw utils::TypeError("list type not support mapget");
}

void naruto::database::ListString::debugString() {
    LOG(INFO) << "";
}
