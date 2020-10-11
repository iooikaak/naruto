//
// Created by 王振奎 on 2020/9/28.
//

#ifndef NARUTO_BUCKETS_H
#define NARUTO_BUCKETS_H

#include <vector>
#include <mutex>
#include <unordered_map>

#include "object.h"

#define DEFAULT_BUCKET_SIZE 16
#define DEFAULT_BUCKET_ELEMENT_SIZE 50000
#define DEFAULT_BUCKET_ELEMENT_COLUMN_SIZE 10

namespace naruto::database{

struct element{
    unsigned encoding;
    std::chrono::steady_clock::time_point create; // 写入时间
    std::chrono::steady_clock::time_point lru; // 最后一次被访问的时间
    std::chrono::steady_clock::time_point expire; // 过期时间
    std::shared_ptr<::naruto::database::object> ptr;
};

class bucket{
    using columns = std::unordered_map<std::string, std::shared_ptr<element>>;
public:
    bucket();
    bucket(const bucket&) = delete;
    bucket& operator= (const bucket&) = delete;

    std::shared_ptr<element> get(const std::string&,const std::string&);
    void put(const std::string&, const std::string&, std::shared_ptr<element>);
    void del(const std::string&,const std::string&);
    int size();
    void flush();

private:
    std::mutex mutex;
    std::shared_ptr<std::unordered_map<std::string, std::shared_ptr<columns>>> objs;
};

class Buckets {
public:
    Buckets();
    Buckets(const Buckets&) = delete;
    Buckets&operator=(const Buckets&) = delete;

//    static const Buckets& instance(){
//        static Buckets buckets;
//        return buckets;
//    }

    std::shared_ptr<element> get(const std::string&,const std::string&);
    void put(const std::string&, const std::string&, std::shared_ptr<element>);
    void del(const std::string&,const std::string&);
    int size();
    void flush();

private:

    std::hash<std::string> _hash;
    int _bucket_size;
    std::vector<std::shared_ptr<bucket>> _buckets;
};

}



#endif //NARUTO_BUCKETS_H
