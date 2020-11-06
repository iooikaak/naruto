//
// Created by kwins on 2020/11/6.
//

#ifndef NARUTO_UT_LIBEV_FILE_H
#define NARUTO_UT_LIBEV_FILE_H

#include <gtest/gtest.h>
#include <ostream>
#include <thread>
#include <stdio.h>
#include <ev++.h>
#include <iostream>

class TestLibevFile: public ::testing::Test{
public:
    static void io_watcher(ev::io& w, int event){
        std::cout << "io_watcher" << std::endl;
    }
};


TEST(TestLibevFile, libev_file){
    ev::io w;
    w.set<&TestLibevFile::io_watcher>(this);
    w.set(ev::default_loop());
    w.start(0, ev::READ);
    ev_run(ev::default_loop());
}

#endif //NARUTO_UT_LIBEV_FILE_H
