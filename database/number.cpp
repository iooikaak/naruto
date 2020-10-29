//
// Created by 王振奎 on 2020/10/6.
//

#include "number.h"

data::TYPE naruto::database::Number::type() {
    return data::NUMBER;
}

void naruto::database::Number::serialize(naruto::utils::Bytes &bytes) {

}

void naruto::database::Number::deSeralize(naruto::utils::Bytes &bytes) {

}

void naruto::database::Number::debugString() {
}

int64_t naruto::database::Number::get() const { return data_.load(); }

void naruto::database::Number::set(const int64_t v) { data_.store(v); }

void naruto::database::Number::incr(const int64_t v) { data_ += v;}
