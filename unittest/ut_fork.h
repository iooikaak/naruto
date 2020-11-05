//
// Created by 王振奎 on 2020/11/1.
//

#ifndef NARUTO_UT_FORK_H
#define NARUTO_UT_FORK_H

#include <gtest/gtest.h>
#include <vector>
#include <thread>
#include <chrono>
#include <algorithm>
#include <iostream>

class TestFork : public ::testing::Test{
public:
    void SetUp(){
        for (int i = 0; i < 10; ++i) {
            vs.push_back(std::to_string(i));
        }
    }
    std::vector<std::string> vs;
};

TEST_F(TestFork, fork){
    pid_t childpid = fork();
    if (childpid == 0){
        std::cout << "child...."<< std::endl;
        std::vector<std::string> childvs;
        std::for_each(vs.begin(), vs.end(), [&childvs](const std::string& v){
            std::this_thread::sleep_for(std::chrono::seconds(1));
            childvs.push_back(v);
            std::cout << "child push:" << v << std::endl;
        });
        std::string str;
        str = "child:";
        std::for_each(childvs.begin(), childvs.end(), [&str](const std::string& v){
            str +=  (v + ",");
        });
        std::cout << str << std::endl;
    }else{
        std::cout << "main...."<< std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::cout << "main....sleep 2 sec"<< std::endl;
        vs[0] = "10000";
        std::string str;
        str = "main:";
        std::for_each(vs.begin(), vs.end(), [&str](const std::string& v){
            str +=  (v + ",");
        });
        std::cout << str << std::endl;
    }
    std::this_thread::sleep_for(std::chrono::seconds(20));
}
#endif //NARUTO_UT_FORK_H
