//
// Created by 王振奎 on 2020/10/6.
//

#include "map.h"

data::TYPE naruto::database::Map::type() { return data::MAP; }

void naruto::database::Map::serialize(data::element&) {

}

void naruto::database::Map::deSeralize(data::element&) {

}

void naruto::database::Map::debugString() {

}

void naruto::database::Map::del(const std::string &key) {
    std::unique_lock lock(mutex_);
    _data.erase(key);
}

std::string naruto::database::Map::get(const std::string &field) {
    std::shared_lock lock(mutex_);

    auto it = _data.find(field);
    if (it != _data.end()) return it->second;
    return "";
}

void naruto::database::Map::put(const std::string &field, const std::string &v) {
    std::unique_lock lock(mutex_);
    _data.insert(std::make_pair(field,v));
}

