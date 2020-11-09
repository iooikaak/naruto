//
// Created by 王振奎 on 2020/10/24.
//

#ifndef NARUTO_PARAMETER_REPL_H
#define NARUTO_PARAMETER_REPL_H

#include <gflags/gflags.h>

// ======================== replication ========================
#define DEFAULT_REPL_BACK_LOG_SIZE (10*1024*1024)
DEFINE_int32(repl_back_log_size, DEFAULT_REPL_BACK_LOG_SIZE, "repl back log fize"); /* NOLINT */
static bool valid_repl_back_log_size(const char* flagname, int value){
    return value > 0;
}
DEFINE_validator(repl_back_log_size,&valid_repl_back_log_size); /* NOLINT */

DEFINE_int32(repl_timeout_sec, 1, "repl timeout sec"); /* NOLINT */
static bool valid_repl_timeout_sec(const char* flagname, int value){
    return value > 0;
}
DEFINE_validator(repl_timeout_sec,&valid_repl_timeout_sec); /* NOLINT */

DEFINE_double(repl_cron_interval, 1, "repl cron interval"); /* NOLINT */
static bool valid_repl_cron_interval(const char* flagname, double value){
    return true;
}
DEFINE_validator(repl_cron_interval,&valid_repl_cron_interval); /* NOLINT */

DEFINE_double(repl_ping_slave_period, 100, "repl ping slave period"); /* NOLINT */
static bool valid_repl_ping_slave_period(const char* flagname, double value){
    return true;
}
DEFINE_validator(repl_ping_slave_period,&valid_repl_ping_slave_period); /* NOLINT */


DEFINE_int32(repl_aof_rotate_size, 10, "aof file rotate size"); /* NOLINT */
static bool valid_repl_aof_rotate_size(const char* flagname, int value){
    return value > 1;
}
DEFINE_validator(repl_aof_rotate_size,&valid_repl_aof_rotate_size); /* NOLINT */

DEFINE_string(repl_aof_dir, "", "aof file rotate size"); /* NOLINT */
static bool valid_repl_aof_dir(const char* flagname, const std::string& value){
    return value != "";
}
DEFINE_validator(repl_aof_dir,&valid_repl_aof_dir); /* NOLINT */

#endif //NARUTO_PARAMETER_REPL_H
