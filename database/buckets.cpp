//
// Created by 王振奎 on 2020/9/28.
//

#include "buckets.h"

#include "utils/pack.h"
#include "number.h"
#include "map.h"
#include "string_.h"
#include "protocol/command_types.pb.h"

// Buckets
naruto::database::Buckets::Buckets(int size) {
    bucket_size_ = size;
    buckets_.resize(bucket_size_);
    for (int i = 0; i < bucket_size_; ++i) {
        buckets_[i] = std::make_shared<bucket>();
    }
}

int naruto::database::Buckets::dump(const std::string& filename) {
    if (size() == 0) return 0;

    std::ofstream out(filename, std::ios::binary|std::ios::out|std::ios::trunc);
    if (!out.is_open()){
        LOG(INFO) << "Db save open file " << filename << " error:" << strerror(errno);
        return -1;
    }
    for (int i = 0; i < bucket_size_; ++i) {
         buckets_[i]->dump(&out);
    }
    auto pos = out.tellp();
    out.close();
    return pos;
}

std::shared_ptr<naruto::database::element> naruto::database::Buckets::get(const std::string & key, const std::string & field) {
    return buckets_[hash_(key) % bucket_size_]->get(key, field);
}

void naruto::database::Buckets::put(const std::string & key, const std::string & field , std::shared_ptr<element> v) {
//    LOG(INFO) << "database put...";
    return buckets_[hash_(key) % bucket_size_]->put(key, field, v);
}

void naruto::database::Buckets::del(const std::string & key, const std::string & field) {
    buckets_[hash_(key) % bucket_size_]->del(key, field);
}

int naruto::database::Buckets::size() {
    int cout = 0;
    for (int i = 0; i < bucket_size_; ++i) {
        cout +=  buckets_[i]->size();
    }
    return cout;
}

void naruto::database::Buckets::flush() {
    for (int i = 0; i < bucket_size_; ++i) { buckets_[i]->flush(); }
}

void naruto::database::Buckets::parse(uint16_t flag, uint16_t type, const unsigned char * msg, size_t body_size) {
    if (type != client::Type::OBJECT) return;

    tensorflow::Features features;
    features.ParseFromArray(msg, int(body_size));
    std::cout << "msg:" << features.DebugString();

    for(const auto& feature : features.feature()){
        auto element = std::make_shared<database::element>();
        element->encoding = feature.second.encoding();
        element->create = time_point<system_clock, milliseconds>(milliseconds(feature.second.create()));
        element->lru = time_point<system_clock, milliseconds>(milliseconds(feature.second.lru()));
        element->expire = time_point<system_clock, milliseconds>(milliseconds(feature.second.expire()));
        switch (feature.second.type()) {
            case tensorflow::MAP:
                element->ptr = std::make_shared<database::Map>();
                break;
            case tensorflow::INT:
                element->ptr = std::make_shared<database::Number<int64_t>>();
                break;
            case tensorflow::INT_LIST:
                element->ptr = std::make_shared<database::ListObject<int64_t>>();
                break;
            case tensorflow::BYTES:
                element->ptr = std::make_shared<database::String>();
                break;
            case tensorflow::BYTES_LIST:
                element->ptr = std::make_shared<database::ListObject<std::string>>();
                break;
            default:
                continue;
        }
        element->ptr->deSeralize(feature.second);
        put(features.id(), feature.first, element);
    }
}

// bucket
naruto::database::bucket::bucket() {
    objs = std::make_shared<std::unordered_map<std::string, std::shared_ptr<columns>>>(DEFAULT_BUCKET_ELEMENT_SIZE);
}

std::shared_ptr<naruto::database::element> naruto::database::bucket::get(const std::string & key, const std::string & field) {
    std::lock_guard<std::mutex> lock(mutex);

    auto it = objs->find(key);
    if (it == objs->end()) return nullptr;

    auto fit = it->second->find(field);
    if (fit == it->second->end()) return nullptr;

    return fit->second;
}

void naruto::database::bucket::put(const std::string & key, const std::string & field, std::shared_ptr<element> v) {
//    LOG(INFO) << "database bucket put...";
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


void naruto::database::bucket::dump(std::ostream* out) {
    for (const auto& object : *objs){
        tensorflow::Features features;
        features.set_id(object.first);
        for (const auto& v : (*object.second)){
            tensorflow::Feature feature;
            feature.set_encoding(v.second->encoding);
            feature.set_create(time_point_cast<milliseconds>(v.second->create).time_since_epoch().count());
            feature.set_lru(time_point_cast<milliseconds>(v.second->lru).time_since_epoch().count());
            feature.set_expire(time_point_cast<milliseconds>(v.second->lru).time_since_epoch().count());
            v.second->ptr->serialize(feature);
            (*features.mutable_feature())[v.first] = std::move(feature);
        }
        utils::Pack::serialize(features, client::Type::OBJECT, out);
    }
}

std::shared_ptr<naruto::database::Buckets> naruto::database::buckets = std::make_shared<naruto::database::Buckets>();
