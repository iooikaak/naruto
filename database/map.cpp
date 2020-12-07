//
// Created by 王振奎 on 2020/10/6.
//

#include "map.h"

naruto::database::Map::Map(const std::unordered_map<std::string, std::string> &data) {
    for(const auto& v : data){
        data_[v.first] = v.second;
    }
}

void naruto::database::Map::serialize(tensorflow::Feature& feature) {
    std::shared_lock lock(mutex_);

    feature.set_type(tensorflow::MAP);
    for(const auto& v : data_){
        (*feature.mutable_bytes_map()->mutable_value())[v.first] = v.second;
    }
}

void naruto::database::Map::deSeralize(const tensorflow::Feature& feature) {
    std::shared_lock lock(mutex_);

    if (feature.type() != tensorflow::MAP) return;
    for (const auto& v : feature.bytes_map().value()){
        data_.emplace(v.first, v.second);
    }
}

void naruto::database::Map::debugString() {

}

void naruto::database::Map::del(const std::string &key) {
    std::unique_lock lock(mutex_);
    data_.erase(key);
}

std::string naruto::database::Map::get(const std::string &field) {
    std::shared_lock lock(mutex_);

    auto it = data_.find(field);
    if (it != data_.end()) return it->second;
    return "";
}

void naruto::database::Map::put(const std::string &field, const std::string &v) {
    std::unique_lock lock(mutex_);
    data_.insert(std::make_pair(field, v));
}

tensorflow::Type naruto::database::Map::type() {
    return tensorflow::Type::MAP;
}

std::string naruto::database::Map::typeName() {
    return tensorflow::Type_descriptor()->FindValueByNumber(type())->name();
}

