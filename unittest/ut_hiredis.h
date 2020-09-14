//
// Created by 王振奎 on 2020/8/12.
//

#ifndef NARUTO_UNITTEST_HIREDIS_H
#define NARUTO_UNITTEST_HIREDIS_H
#include <gtest/gtest.h>
#include <hiredis/hiredis.h>
#include <iostream>

class TestHiRedis : public ::testing::Test{
public:
    virtual void SetUp(){
        _c = redisConnect("127.0.0.1",6379);
        if (_c == NULL || _c->err){
            if (_c){
                std::cout << "redis connect error:" << _c->errstr << std::endl;
            }else{
                std::cout << "redis connect can not allocate redis context" << std::endl;
            }
        }
    }

    virtual void TearDown(){
        redisFree(_c);
    }

    redisContext* _c;
};

TEST_F(TestHiRedis, hi_redis){
    redisCommand(_c, "SET foo111 bar");
}

#endif //NARUTO_UNITTEST_HIREDIS_H
