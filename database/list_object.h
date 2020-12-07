//
// Created by kwins on 2020/10/28.
//

#ifndef NARUTO_LIST_OBJECT_H
#define NARUTO_LIST_OBJECT_H

#include <vector>
#include <shared_mutex>
#include <type_traits>

#include "object.h"
#include "utils/errors.h"

namespace naruto::database {

template <typename T>
class listBase{
public:
    explicit listBase(std::vector<T> data = {}){
        for(const auto& v : data){
            data_.push_back(v);
        }
    }

    int len(){
        return data_.size();
    }

    T lpop(){
        std::unique_lock lock(mutex_);

        T v = data_[data_.size()];
        data_.pop_back();
        return v;
    }

    void ltrim(int start, int end){
        std::unique_lock lock(mutex_);

        if (data_.empty()) return;

        if (start > end || start < 0 || end < 0)
            throw utils::BadArgError("ltrim index end must great start");

        if (end > data_.size()) {
            if (start == end){
                start = data_.size();
            }
            end = data_.size();
        }

        auto it = data_.begin();
        for (it = it + start; it < it + end; ++it){
            data_.erase(it);
        }
    }

    void lpush(const T& v){
        std::unique_lock lock(mutex_);
        data_.push_back(v);
    }

    void lrange(int start, int end, std::vector<T>& reply) const{
        std::shared_lock lock(mutex_);

        if (data_.empty()) return;

        if (start > end || start < 0 || end < 0)
            throw utils::BadArgError("lrange index end must great start");

        if (end > data_.size()) {
            if (start == end){
                start = data_.size();
            }
            end = data_.size();
        }

        reply.reserve(end - start + 1);
        for (int i = start; i < end; ++i) {
            reply.push_back(data_[i]);
        }
    }
protected:
    std::shared_mutex mutex_;
    std::vector<T> data_;
};

template <typename T>
class ListObject : public object, public listBase<T> {
    static_assert(
            std::is_same<int64_t, T>::value ||
            std::is_same<std::string, T>::value ||
            std::is_same<float, T>::value ,"T must be one of int64_t,float,string");
public:
    explicit ListObject(const std::vector<T>& data = {}) : listBase<T>(data){}

    tensorflow::Type type() override {
        return tensorflow::Type::UNKNOW;
    }

    std::string typeName() override {
        return tensorflow::Type_descriptor()->FindValueByNumber(type())->name();
    }

    void serialize(tensorflow::Feature& v) override{}

    void deSeralize(const tensorflow::Feature& v) override{}

    void debugString() override{ }
};

template <>
class ListObject<int64_t> : public object, public listBase<int64_t>{
public:
    explicit ListObject(const std::vector<int64_t>& data = {}) : listBase<int64_t>(data){}

    tensorflow::Type type() override {
        return tensorflow::INT_LIST;
    }

    std::string typeName() override {
        return tensorflow::Type_descriptor()->FindValueByNumber(type())->name();
    }

    void serialize(tensorflow::Feature &feature) override {
        feature.set_type(tensorflow::INT_LIST);
        for(const auto& v : data_){
            feature.mutable_int64_list()->add_value(v);
        }
    }

    void deSeralize(const tensorflow::Feature &feature) override {
        if (feature.type() != tensorflow::INT_LIST) return;
        for (int i = 0; i < feature.int64_list().value_size(); ++i) {
            data_.push_back(feature.int64_list().value(i));
        }
    }

    void debugString() override {

    }
};

template <>
class ListObject<float> : public object, public listBase<float>{
public:
    explicit ListObject(const std::vector<float>& data = {}) : listBase<float>(data){}

    tensorflow::Type type() override {
        return tensorflow::FLOAT_LIST;
    }

    std::string typeName() override {
        return tensorflow::Type_descriptor()->FindValueByNumber(type())->name();
    }

    void serialize(tensorflow::Feature &feature) override {
        feature.set_type(tensorflow::FLOAT_LIST);
        for(const auto& v : data_){
            feature.mutable_float_list()->add_value(v);
        }
    }

    void deSeralize(const tensorflow::Feature &feature) override {
        if (feature.type() != tensorflow::FLOAT_LIST) return;
        for (int i = 0; i < feature.float_list().value_size(); ++i) {
            data_.push_back(feature.float_list().value(i));
        }
    }

    void debugString() override {}
};


template <>
class ListObject<std::string> : public object, public listBase<std::string>{
public:
    explicit ListObject(const std::vector<std::string>& data = {}) : listBase<std::string>(data){}

    tensorflow::Type type() override {
        return tensorflow::BYTES_LIST;
    }

    std::string typeName() override {
        return tensorflow::Type_descriptor()->FindValueByNumber(type())->name();
    }

    void serialize(tensorflow::Feature &feature) override {
        feature.set_type(tensorflow::BYTES_LIST);
        for(const auto& v : data_){
            feature.mutable_bytes_list()->add_value(v);
        }
    }

    void deSeralize(const tensorflow::Feature &feature) override {
        if (feature.type() != tensorflow::BYTES_LIST) return;
        for (int i = 0; i < feature.bytes_list().value_size(); ++i) {
            data_.push_back(feature.bytes_list().value(i));
        }
    }

    void debugString() override {}
};

}



#endif //NARUTO_LIST_OBJECT_H
