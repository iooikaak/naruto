//
// Created by kwins on 2020/11/6.
//

#ifndef NARUTO_UT_ROTATE_FILE_STREAM_H
#define NARUTO_UT_ROTATE_FILE_STREAM_H

#include <gtest/gtest.h>
#include "sink/rotate_file_stream.h"

class TestRotateFileStream : public ::testing::Test{
public:

};

TEST_F(TestRotateFileStream, rotate_file_stream){
    naruto::sink::RotateFileStream stream("/Users/kwins/Project/cproj/naruto/test",20);
    for (int i = 0; i < 100; ++i) {
        stream.write("xxxxx",5);
    }
    stream.flush();
}

#endif //NARUTO_UT_ROTATE_FILE_STREAM_H
