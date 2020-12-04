//
// Created by 王振奎 on 2020/10/24.
//

#ifndef NARUTO_PARAMETER_REPL_H
#define NARUTO_PARAMETER_REPL_H

#include <gflags/gflags.h>

// ======================== replication ========================
// 复制积压缓冲大小
#define DEFAULT_REPL_BACK_LOG_SIZE (10*1024*1024)
#define DEFAULT_REPL_AOF_ROTATE_SIZE (500 * 1024 * 1024)
DEFINE_int32(repl_back_log_size, DEFAULT_REPL_BACK_LOG_SIZE, "repl back log fize"); /* NOLINT */
static bool valid_repl_back_log_size(const char* flagname, int value){
    return value > 0;
}
DEFINE_validator(repl_back_log_size,&valid_repl_back_log_size); /* NOLINT */

// 复制协议超时时间
DEFINE_int32(repl_timeout_sec, 10, "repl timeout sec"); /* NOLINT */
static bool valid_repl_timeout_sec(const char* flagname, int value){
    return value > 0;
}
DEFINE_validator(repl_timeout_sec,&valid_repl_timeout_sec); /* NOLINT */

// onReplCon 执行间隔
DEFINE_double(repl_cron_interval, 1, "repl cron interval"); /* NOLINT */
static bool valid_repl_cron_interval(const char* flagname, double value){
    return true;
}
DEFINE_validator(repl_cron_interval,&valid_repl_cron_interval); /* NOLINT */

// master ping slave 时间周期
DEFINE_double(repl_ping_slave_period, 1, "repl ping slave period, ms"); /* NOLINT */
static bool valid_repl_ping_slave_period(const char* flagname, double value){
    return true;
}
DEFINE_validator(repl_ping_slave_period,&valid_repl_ping_slave_period); /* NOLINT */

// aof 文件大小
DEFINE_int32(repl_aof_rotate_size, DEFAULT_REPL_AOF_ROTATE_SIZE, "aof file rotate size"); /* NOLINT */
static bool valid_repl_aof_rotate_size(const char* flagname, int value){
    return value > 1;
}
DEFINE_validator(repl_aof_rotate_size,&valid_repl_aof_rotate_size); /* NOLINT */

// 持久化文件目录
DEFINE_string(repl_dir, ".", "repl file dir"); /* NOLINT */
static bool valid_repl_dir(const char* flagname, const std::string& value){
    return !value.empty();
}
DEFINE_validator(repl_dir,&valid_repl_dir); /* NOLINT */

// 复制状态文件，存储服务运行过程中一些需要持久化的配置
DEFINE_string(repl_conf_filename, "repl.conf", "repl conf file name"); /* NOLINT */
static bool valid_repl_conf_filename(const char* flagname, const std::string& value){
    return !value.empty();
}
DEFINE_validator(repl_conf_filename,&valid_repl_conf_filename); /* NOLINT */

// 复制状态文件刷新间隔 sec
DEFINE_double(repl_conf_flush_interval, 1, "repl conf flush interval"); /* NOLINT */
static bool valid_repl_conf_flush_interval(const char* flagname, double value){
    return value > 0;
}
DEFINE_validator(repl_conf_flush_interval,&valid_repl_conf_flush_interval); /* NOLINT */

// bgsve sec
DEFINE_double(repl_bgsave_interval, 60, "repl bgsave interval"); /* NOLINT */
static bool valid_repl_bgsave_interval(const char* flagname, double value){
    return value > 0;
}
DEFINE_validator(repl_bgsave_interval,&valid_repl_bgsave_interval); /* NOLINT */

// 复制状态文件刷新间隔 sec
DEFINE_double(repl_aof_interval, 1, "repl conf flush interval"); /* NOLINT */
static bool valid_repl_aof_interval(const char* flagname, double value){
    return value > 0;
}
DEFINE_validator(repl_aof_interval,&valid_repl_aof_interval); /* NOLINT */


// database 文件名
DEFINE_string(repl_database_filename, "naruto.db", "repl database file name"); /* NOLINT */
static bool valid_repl_database_filename(const char* flagname, const std::string& value){
    return !value.empty();
}
DEFINE_validator(repl_database_filename,&valid_repl_database_filename); /* NOLINT */



#endif //NARUTO_PARAMETER_REPL_H
