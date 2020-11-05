//
// Created by 王振奎 on 2020/9/28.
//

#ifndef NARUTO_BUCKETS_H
#define NARUTO_BUCKETS_H

#include <vector>
#include <mutex>
#include <unordered_map>
#include <ostream>
#include <fstream>

#include "object.h"
#include "protocol/data.pb.h"
#include "protocol/client.pb.h"

#define DEFAULT_BUCKET_SIZE 16
#define DEFAULT_BUCKET_ELEMENT_SIZE 50000
#define DEFAULT_BUCKET_ELEMENT_COLUMN_SIZE 10

namespace naruto::database{

struct element{
    unsigned encoding;
    std::chrono::steady_clock::time_point create; // 写入时间
    std::chrono::steady_clock::time_point lru; // 最后一次被访问的时间
    std::chrono::steady_clock::time_point expire; // 过期时间
    std::shared_ptr<object> ptr;

    template<typename T>
    std::shared_ptr<T> cast(){ return std::dynamic_pointer_cast<T>(ptr); };
};

class bucket{
public:
    using columns = std::unordered_map<std::string, std::shared_ptr<element>>;
    using row = std::unordered_map<std::string, std::shared_ptr<columns>>;

    bucket();
    bucket(const bucket&) = delete;
    bucket& operator= (const bucket&) = delete;
    void dump(std::ostream* out);

    std::shared_ptr<element> get(const std::string&,const std::string&);
    void put(const std::string&, const std::string&, std::shared_ptr<element>);
    void del(const std::string&,const std::string&);
    int size();
    void flush();

private:
    std::mutex mutex;
    std::shared_ptr<row> objs;
};

class Buckets {
public:
    explicit Buckets(int size = DEFAULT_BUCKET_SIZE);
    Buckets(const Buckets&) = delete;
    Buckets&operator=(const Buckets&) = delete;

    int dump(const std::string& filename);
    int getBucketSize() const;
    [[nodiscard]] const std::vector<std::shared_ptr<bucket>> &getBuckets() const;
    std::shared_ptr<element> get(const std::string&,const std::string&);
    void put(const std::string&, const std::string&, std::shared_ptr<element>);
    void del(const std::string&,const std::string&);
    int size();
    void flush();

private:
    std::hash<std::string> hash_;
    int bucket_size_;
    std::vector<std::shared_ptr<bucket>> buckets_;
};

}



#endif //NARUTO_BUCKETS_H
