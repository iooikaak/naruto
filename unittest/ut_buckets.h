//
// Created by kwins on 2020/11/18.
//

#ifndef NARUTO_UT_BUCKETS_H
#define NARUTO_UT_BUCKETS_H
#include <gtest/gtest.h>
#include <chrono>
#include "database/buckets.h"
#include "database/number.h"
#include "database/string_.h"
#include "database/map.h"

class TestBuckets : public ::testing::Test{
public:
    void genTestData(){
        for (int i = 0; i < 7; ++i) {
            auto element = std::make_shared<naruto::database::element>();
            element->create = std::chrono::system_clock::now();
            element->lru = std::chrono::system_clock::now();
            element->expire = element->create + std::chrono::seconds(10);
            element->encoding = 1;
            switch (i % 7) {
                case 0:
                    element->ptr = std::make_shared<naruto::database::ListObject<std::string>>(std::vector<std::string>{"11222","2","3","4","5"});
                    break;
                case 1:
                    element->ptr = std::make_shared<naruto::database::ListObject<int64_t>>(std::vector<int64_t>{1,2,3,4,5});
                    break;
                case 2:
                    element->ptr = std::make_shared<naruto::database::ListObject<float>>(std::vector<float>{1.0,2.0,3.0,4.2,5.3});
                    break;
                case 3:
                    element->ptr = std::make_shared<naruto::database::Number<int64_t>>(45);
                    break;
                case 4:
                    element->ptr = std::make_shared<naruto::database::Number<float>>(4.5);
                    break;
                case 5:
                    element->ptr = std::make_shared<naruto::database::String>("xxxcccc");
                    break;
                case 6:
                    std::unordered_map<std::string,std::string> ms({ {"x","1"}, {"y","2"} });
                    element->ptr = std::make_shared<naruto::database::Map>(ms);
                    break;
            }
            buckets_.put("test_" + std::to_string(i),"field_" + std::to_string(i), element);
        }
        buckets_.dump("naruto.db");
    }
    void loadTestData(){
        buckets_.load("naruto.db");
    }

    naruto::database::Buckets buckets_{};
};

TEST_F(TestBuckets, buckets){
    genTestData();
    loadTestData();
    LOG(INFO) << "size:" << buckets_.size();
}

#endif //NARUTO_UT_BUCKETS_H
