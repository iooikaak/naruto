//
// Created by 王振奎 on 2020/10/6.
//

#include "string_.h"

void naruto::database::String::serialize(tensorflow::Feature& feature) {
    feature.set_type(tensorflow::BYTES);
    feature.mutable_bytes_value()->set_value(data_);
}

void naruto::database::String::debugString() {
    LOG(INFO) << "database debug string data:" << data_;
}

void naruto::database::String::deSeralize(const tensorflow::Feature& feature) {
    if (feature.type() != tensorflow::BYTES) return;
    data_ = feature.bytes_value().value();
}

std::string naruto::database::String::get() { return data_; }

void naruto::database::String::set(const std::string &value) { data_ = value; }