//
// Created by 王振奎 on 2020/10/6.
//

#ifndef NARUTO_NUMBER_H
#define NARUTO_NUMBER_H

#include <atomic>
#include <type_traits>
#include <shared_mutex>

#include "object.h"
#include "utils/errors.h"

namespace naruto::database {

template <typename T>
class Number : public object{
    static_assert(std::is_same<int64_t, T>::value||std::is_same<float,T>::value,
            "T must be one of int64 or float");
public:
    explicit Number(T v = 0) : data_(v){}

    void serialize(tensorflow::Feature& feature) override{
        feature.set_type(tensorflow::INT);
        feature.mutable_int64_value()->set_value(data_);
    }

    void deSeralize(const tensorflow::Feature& feature) override{
        if (feature.type() != tensorflow::INT) return;
        data_.store(feature.int64_value().value());
    }

    void debugString() override{

    }

    T get() const{ return data_.load(); }

    void set(T v){
        LOG(INFO) << "Number set -->>" << v;
        data_.store(v);
    }

    void incr(T v){ data_ += v; }

    ~Number() override = default;

private:
    std::atomic<T> data_;
};

template <>
class Number<float> : public object{
public:
    explicit Number(float v = 0) : data_(v){}

    void serialize(tensorflow::Feature& feature) override{
        std::shared_lock lock(mutex_);

        feature.set_type(tensorflow::FLOAT);
        feature.mutable_float_value()->set_value(data_);
    }

    void deSeralize(const tensorflow::Feature& feature) override{
        std::shared_lock lock(mutex_);

        if (feature.type() != tensorflow::FLOAT) return;
        data_ =  feature.float_value().value();
    }

    void debugString() override{

    }

    float get() {
        std::shared_lock lock(mutex_);
        return data_;
    }

    void set(float v){
        std::unique_lock lock(mutex_);
        data_ = v;
    }

    void incr(float v){
        std::unique_lock lock(mutex_);
        data_ += v;
    }

    ~Number() override = default;

private:
    std::shared_mutex mutex_;
    float data_;

};

}

#endif //NARUTO_NUMBER_H
