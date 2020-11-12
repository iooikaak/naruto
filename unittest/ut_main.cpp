//
// Created by 王振奎 on 2020/8/10.
//

#include <gtest/gtest.h>
#include <gflags/gflags.h>
//#include "ut_ev_accept.h"
//#include "ut_hiredis.h"
//#include "ut_bytes.h"
//#include "ut_cpp.h"
//#include "ut_cpp.h"
//#include "ut_chrono.h"
//#include "ut_client.h"
//#include "ut_fork.h"
//#include "ut_ofstream.h"
//#include "ut_client_slaveof.h"
//#include "ut_libev_file.h"
//#include "ut_rotate_file_stream.h"
#include "ut_repl_backlog_feed.h"
int main(int argc, char* argv[]){
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}