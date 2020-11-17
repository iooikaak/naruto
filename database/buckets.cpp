//
// Created by 王振奎 on 2020/9/28.
//

#include "buckets.h"
#include "utils/pack.h"

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

    std::fstream out(filename, std::ios::binary|std::ios::out|std::ios::trunc);
    if (!out.is_open()){
        LOG(INFO) << "Db save open file " << tmpfile << " error:" << strerror(errno);
        return -1;
    }
    for (int i = 0; i < bucket_size_; ++i) {
         buckets_[i]->dump(&out);
    }
    out.close();
    return 0;
}

std::shared_ptr<naruto::database::element> naruto::database::Buckets::get(const std::string & key, const std::string & field) {
    return buckets_[hash_(key) % bucket_size_]->get(key, field);
}

void naruto::database::Buckets::put(const std::string & key, const std::string & field , std::shared_ptr<element> v) {
    LOG(INFO) << "database put...";
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

int naruto::database::Buckets::getBucketSize() const {
    return bucket_size_;
}

const std::vector<std::shared_ptr<naruto::database::bucket>> &naruto::database::Buckets::getBuckets() const { return buckets_; }

naruto::database::bucket::bucket() {
    objs = std::make_shared<std::unordered_map<std::string, std::shared_ptr<columns>>>(DEFAULT_BUCKET_ELEMENT_SIZE);
}

// bucket
void naruto::database::bucket::dump(std::ostream* out) {
    for (const auto& object : *objs){
        data::object row;
        row.set_key(object.first);
        for (const auto& v : (*object.second)){
            auto column = row.add_columns();
            column->set_field(v.first);
            auto element =  column->mutable_value();
            element->set_type(v.second->ptr->type());
            v.second->ptr->serialize(*element);
        }
        utils::Pack::serialize(row, client::OBJECT, out);
    }
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


std::shared_ptr<naruto::database::Buckets> naruto::database::buckets = std::make_shared<naruto::database::Buckets>();
