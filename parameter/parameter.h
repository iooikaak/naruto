//
// Created by 王振奎 on 2020/10/24.
//

#ifndef NARUTO_NARUTO_PARAMETER_H
#define NARUTO_NARUTO_PARAMETER_H

#include <gflags/gflags.h>

// ======================== naruto ========================
DEFINE_int32(port, 7290, "listen port"); /* NOLINT */
static bool valid_port(const char* flagname, int value){
    LOG(INFO) << "argument port:" << value;
    return true;
}
DEFINE_validator(port,&valid_port); /* NOLINT */

DEFINE_int32(cron_interval,1, "server time cron seconds"); /* NOLINT */
static bool valid_cron_interval(const char* flagname, int value){
    LOG(INFO) << "argument cron_interval:" << value;
    return true;
}
DEFINE_validator(cron_interval,&valid_cron_interval); /* NOLINT */

DEFINE_int32(tcp_backlog, 521, "tcp back log"); /* NOLINT */
static bool valid_tcp_backlog(const char* flagname, int value){
    LOG(INFO) << "argument tcp_backlog:" << value;
    return true;
}
DEFINE_validator(tcp_backlog,&valid_tcp_backlog); /* NOLINT */

DEFINE_int32(bucket_num, 16, "database bucket num"); /* NOLINT */
static bool valid_bucket_num(const char* flagname, int value){
    LOG(INFO) << "argument bucket_num:" << value;
    return true;
}
DEFINE_validator(bucket_num,&valid_bucket_num); /* NOLINT */


#endif //NARUTO_NARUTO_PARAMETER_H
