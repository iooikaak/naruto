//
// Created by kwins on 2020/10/28.
//

#include "list_object.h"

template<typename T, data::TYPE DT>
void naruto::database::ListObject<T, DT>::serialize(naruto::utils::Bytes &bytes) {

}

template<typename T, data::TYPE DT>
void naruto::database::ListObject<T, DT>::deSeralize(naruto::utils::Bytes &bytes) {

}

template<typename T, data::TYPE DT>
void naruto::database::ListObject<T, DT>::debugString() {

}

template<typename T, data::TYPE DT>
int naruto::database::ListObject<T, DT>::len() { return data_.size(); }

template<typename T, data::TYPE DT>
T naruto::database::ListObject<T, DT>::lpop() {
    std::unique_lock lock(mutex_);

    T v = data_[data_.size()];
    data_.pop_back();
    return v;
}

template<typename T, data::TYPE DT>
void naruto::database::ListObject<T, DT>::ltrim(int start, int end) {
    std::unique_lock lock(mutex_);

    if (data_.empty()) return;

    if (start > end || start < 0 || end < 0) throw utils::BadArgError("ltrim index end must great start");

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

template<typename T, data::TYPE DT>
void naruto::database::ListObject<T, DT>::lpush(const T & v) {
    std::unique_lock lock(mutex_);
    data_.push_back(v);
}

template<typename T, data::TYPE DT>
void naruto::database::ListObject<T, DT>::lrange(int start, int end, std::vector<T> &reply) {
    std::shared_lock lock(mutex_);

    if (data_.empty()) return;

    if (start > end || start < 0 || end < 0) throw utils::BadArgError("lrange index end must great start");

    if (end > data_.size()) {
        if (start == end){
            start = data_.size();
        }
        end = data_.size();
    }

    reply.reserve(data_.size());
    for (int i = start; i < end; ++i) {
        reply.push_back(data_[i]);
    }
}
