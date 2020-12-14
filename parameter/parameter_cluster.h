//
// Created by kwins on 2020/12/10.
//

#ifndef NARUTO_PARAMETER_CLUSTER_H
#define NARUTO_PARAMETER_CLUSTER_H
#include <gflags/gflags.h>

DEFINE_int32(cluster_handshake_timeout_ms, 200, "cluster handshake timeout ms"); /* NOLINT */
static bool valid_cluster_handshake_timeout_ms(const char* flagname, int value){
    return value > 0;
}
DEFINE_validator(cluster_handshake_timeout_ms,&valid_cluster_handshake_timeout_ms); /* NOLINT */

// 集群节点通信超时
DEFINE_int32(cluster_node_timeout_sec, 10, "cluster node timeout ms"); /* NOLINT */
static bool valid_cluster_node_timeout_sec(const char* flagname, int value){
    return value > 0;
}
DEFINE_validator(cluster_node_timeout_sec,&valid_cluster_node_timeout_sec); /* NOLINT */

// 集群写保护时间
DEFINE_int32(cluster_writable_delay_ms, 2000, "cluster write dealy timeout ms"); /* NOLINT */
static bool valid_cluster_writable_delay_ms(const char* flagname, int value){
    return value > 0;
}
DEFINE_validator(cluster_writable_delay_ms,&valid_cluster_writable_delay_ms); /* NOLINT */

#endif //NARUTO_PARAMETER_CLUSTER_H
