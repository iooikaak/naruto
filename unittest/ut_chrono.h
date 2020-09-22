//
// Created by 王振奎 on 2020/9/2.
//

#ifndef NARUTO_UT_CHRONO_H
#define NARUTO_UT_CHRONO_H

#include <gtest/gtest.h>
#include <glog/logging.h>
#include <thread>

class TestChrono : public ::testing::Test{

};

TEST_F(TestChrono, test_chrono){
    std::chrono::system_clock::time_point tp_epoch = std::chrono::system_clock::now();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::chrono::system_clock::time_point tp_epoch1 = std::chrono::system_clock::now();

    auto time_span = std::chrono::duration_cast<std::chrono::duration<double >>(tp_epoch1-tp_epoch);
    LOG(INFO) << "time_span:" << int(time_span.count());
    std::time_t tt = std::chrono::system_clock::to_time_t(tp_epoch);
    LOG(INFO) << "tp_epoch:" << ctime(&tt);
    LOG(INFO) <<"tp_epoch.time_since_epoch().count():" << tp_epoch.time_since_epoch().count();
}

#endif //NARUTO_UT_CHRONO_H
