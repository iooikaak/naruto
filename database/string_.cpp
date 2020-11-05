//
// Created by 王振奎 on 2020/10/6.
//

#include "string_.h"

data::TYPE naruto::database::String::type() { return data::STRING; }

void naruto::database::String::serialize(data::element& element) { element.set_str(data_); }

void naruto::database::String::debugString() {
    LOG(INFO) << "database debug string data:" << data_;
}

void naruto::database::String::deSeralize(data::element& element) { data_ = element.str(); }

std::string naruto::database::String::get() { return data_; }

void naruto::database::String::set(const std::string &value) { data_ = value; }