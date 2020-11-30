//
// Created by kwins on 2020/11/26.
//

#ifndef NARUTO_UT_UTILS_H
#define NARUTO_UT_UTILS_H

#include <gtest/gtest.h>
#include "utils/basic.h"
#include <glog/logging.h>

TEST(TestUtils, utils){
    LOG(INFO) << naruto::utils::Basic::genRandomID(DEFAULT_RUN_ID_LEN);
}
#endif //NARUTO_UT_UTILS_H
