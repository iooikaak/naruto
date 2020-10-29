//
// Created by 王振奎 on 2020/9/28.
//

#include "buckets.h"

// Buckets
naruto::database::Buckets::Buckets() {
    _bucket_size = DEFAULT_BUCKET_SIZE;
    _buckets.resize(_bucket_size);
    for (int i = 0; i < _bucket_size; ++i) {
        _buckets[i] = std::make_shared<bucket>();
    }
}

std::shared_ptr<naruto::database::element> naruto::database::Buckets::get(const std::string & key, const std::string & field) {
    return _buckets[_hash(key) % _bucket_size]->get(key, field);
}

void naruto::database::Buckets::put(const std::string & key, const std::string & field , std::shared_ptr<element> v) {
    LOG(INFO) << "database put...";
    return _buckets[_hash(key) % _bucket_size]->put(key, field, v);
}

void naruto::database::Buckets::del(const std::string & key, const std::string & field) {
    _buckets[_hash(key) % _bucket_size]->del(key, field);
}

int naruto::database::Buckets::size() {
    int cout = 0;
    for (int i = 0; i < _bucket_size; ++i) {
        cout +=  _buckets[i]->size();
    }
    return cout;
}

void naruto::database::Buckets::flush() {
    for (int i = 0; i < _bucket_size; ++i) { _buckets[i]->flush(); }
}

int naruto::database::Buckets::getBucketSize() const {
    return _bucket_size;
}

const std::vector<std::shared_ptr<naruto::database::bucket>> &naruto::database::Buckets::getBuckets() const { return _buckets; }

naruto::database::bucket::bucket() {
    objs = std::make_shared<std::unordered_map<std::string, std::shared_ptr<columns>>>(DEFAULT_BUCKET_ELEMENT_SIZE);
}

// bucket
std::shared_ptr<naruto::database::bucket::row> naruto::database::bucket::objects() const { return objs; }

std::shared_ptr<naruto::database::element> naruto::database::bucket::get(const std::string & key, const std::string & field) {
    std::lock_guard<std::mutex> lock(mutex);

    auto it = objs->find(key);
    if (it == objs->end()) return nullptr;

    auto fit = it->second->find(field);
    if (fit == it->second->end()) return nullptr;

    return fit->second;
}

void naruto::database::bucket::put(const std::string & key, const std::string & field, std::shared_ptr<element> v) {
    LOG(INFO) << "database bucket put...";
    std::lock_guard<std::mutex> lock(mutex);

    auto it = objs->find(key);
    if (it == objs->end()){
        auto columns_ptr = std::make_shared<columns>(DEFAULT_BUCKET_ELEMENT_COLUMN_SIZE);
        columns_ptr->emplace(field, v);
        objs->emplace(key, columns_ptr);
    }else{
        it->second->emplace(field, v);
    }
}

void naruto::database::bucket::del(const std::string & key, const std::string & field) {
    std::lock_guard<std::mutex> lock(mutex);

    auto it = objs->find(key);
    if (it == objs->end()) return;

    it->second->erase(field);
}

int naruto::database::bucket::size() { return objs->size(); }

void naruto::database::bucket::flush() { objs = std::make_shared<std::unordered_map<std::string, std::shared_ptr<columns>>>(); }
