//
// Created by kwins on 2020/11/5.
//

#ifndef NARUTO_UT_OFSTREAM_H
#define NARUTO_UT_OFSTREAM_H

#include <ostream>
#include <gtest/gtest.h>
#include <fstream>
#include <dirent.h>
class TestFstream : public ::testing::Test{
public:

};

TEST_F(TestFstream, ofstream){
    std::ofstream out("a.txt", std::ofstream::app|std::ofstream::out);
    out << "aaaaa";
    long long pos = out.tellp();
    std::cout <<"pos:" << pos << std::endl;

    unsigned char isFile =0x8;
    unsigned char isDir =0x4;
    DIR* dir = opendir(".");
    dirent* ptr;
    while ((ptr = readdir(dir)) != nullptr){
        if (ptr->d_type == isFile){
            std::cout << "name:" << ptr->d_name << " type file"  << std::endl;
        } else if (ptr->d_type == isDir){
            std::cout << "name:" << ptr->d_name << " type dir" << std::endl;
        }
    }
}
#endif //NARUTO_UT_OFSTREAM_H
