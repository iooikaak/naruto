//
// Created by 王振奎 on 2020/8/13.
//

#ifndef NARUTO_UNITTEST_UT_BYTES_H
#define NARUTO_UNITTEST_UT_BYTES_H
#include <gtest/gtest.h>
#include "utils/bytes.h"
#include <string>

class TestBytes : public ::testing::Test{
public:
    virtual void SetUp(){

    }
    naruto::utils::Bytes _bytes;
};

TEST_F(TestBytes, bytes){
    _bytes.putInt(123456);
    _bytes.putInt(1);
    std::string str("哈哈哈哈哈哈哈");
    _bytes.putBytes((uint8_t*)str.c_str(), str.size());

    uint32_t length = _bytes.getInt();
    uint32_t type = _bytes.getInt();
    uint8_t* buf = (uint8_t*)malloc(str.size());
    _bytes.getBytes(buf, str.size());
    char* getstr = reinterpret_cast<char*>(buf);
    std::cout << "length:" << length << " type:" << type << " str:" << getstr << " bytes size:" << _bytes.size() << std::endl;

    _bytes.clear();
    std::cout << "after clean size:" << _bytes.size() <<  std::endl;
}

#endif //NARUTO_UNITTEST_UT_BYTES_H
