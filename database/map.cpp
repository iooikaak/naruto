//
// Created by 王振奎 on 2020/10/6.
//

#include "map.h"

data::TYPE naruto::database::Map::type() { return data::MAP; }

void naruto::database::Map::get(data::VALUE &value) {
    value.set_type(type());
    auto kvs = value.mutable_kvs();
    for (const auto& v : _data){
        (*kvs)[v.first] = v.second;
    }
}

void naruto::database::Map::set(const data::VALUE &value) {
    if (value.type() != type()) throw utils::TypeError("data type must map");
    for(auto v : value.kvs()){
        _data.insert(std::make_pair(v.first,v.second));
    }
}

void naruto::database::Map::serialize(utils::Bytes &bytes) {

}

int naruto::database::Map::len() { return _data.size(); }

void naruto::database::Map::lpop(data::VALUE &value) {
    throw utils::TypeError("map type not support lpop");
}

void naruto::database::Map::ltrim(int start, int end) {
    throw utils::TypeError("float type not support ltrim");
}

void naruto::database::Map::lpush(const data::VALUE &value) {
    throw utils::TypeError("float type not support lpush");
}

void naruto::database::Map::lrange(int start, int end, data::VALUE &reply) {
    throw utils::TypeError("float type not support lrange");
}

void naruto::database::Map::incr(int v) {
    throw utils::TypeError("float type not support incr");
}

void naruto::database::Map::mapdel(const std::string &key) { _data.erase(key); }

naruto::database::Map::~Map() {

}

void naruto::database::Map::debugString() {

}
