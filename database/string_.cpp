//
// Created by 王振奎 on 2020/10/6.
//

#include "string_.h"

data::TYPE naruto::database::String::type() { return data::STRING; }

void naruto::database::String::serialize(::naruto::utils::Bytes &bytes) {

}

void naruto::database::String::debugString() {
    LOG(INFO) << "database debug string data:" << _data;
}

void naruto::database::String::deSeralize(naruto::utils::Bytes &bytes) {

}

std::string naruto::database::String::get() { return _data; }

void naruto::database::String::set(const std::string &value) { _data = value; }