//
// Created by 王振奎 on 2020/9/2.
//

#ifndef NARUTO_UT_CHRONO_H
#define NARUTO_UT_CHRONO_H

#include <gtest/gtest.h>
#include <glog/logging.h>
class TestChrono : public ::testing::Test{

};

TEST_F(TestChrono, test_chrono){
    std::chrono::system_clock::time_point tp_epoch = std::chrono::system_clock::now();
    std::time_t tt = std::chrono::system_clock::to_time_t(tp_epoch);
    LOG(INFO) << "tp_epoch:" << ctime(&tt);
}

#endif //NARUTO_UT_CHRONO_H
