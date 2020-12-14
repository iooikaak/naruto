//
// Created by kwins on 2020/12/8.
//

#include "cluster_node.h"
#include "protocol/cluster_message.pb.h"
#include "protocol/command_types.pb.h"

void naruto::cluster::clusterNode::desc(::cluster::command_node_data& info) {
    info.set_addr(addr);
    info.set_ctime(std::chrono::duration_cast<std::chrono::seconds>(ctime.time_since_epoch()).count());
    info.set_name(name);
    info.set_flags(flags);
    if (isFail()){
        info.add_flags_name(cmdtype::Type_descriptor()->FindValueByNumber((int)clusterLink::flags::FAIL)->name());
    }
    if (isPFail()){
        info.add_flags_name(cmdtype::Type_descriptor()->FindValueByNumber((int)clusterLink::flags::PFAIL)->name());
    }
    if (isMaster()){
        info.add_flags_name(cmdtype::Type_descriptor()->FindValueByNumber((int)clusterLink::flags::MASTER)->name());
    }
    if (isSlave()){
        info.add_flags_name(cmdtype::Type_descriptor()->FindValueByNumber((int)clusterLink::flags::SLAVE)->name());
    }
    if (isHandshake()){
        info.add_flags_name(cmdtype::Type_descriptor()->FindValueByNumber((int)clusterLink::flags::HANDSHAKE)->name());
    }
    if (isMyself()){
        info.add_flags_name(cmdtype::Type_descriptor()->FindValueByNumber((int)clusterLink::flags::MYSELF)->name());
    }
    if (isMeet()){
        info.add_flags_name(cmdtype::Type_descriptor()->FindValueByNumber((int)clusterLink::flags::MEET)->name());
    }
    info.set_config_epoch(config_epoch);
    for (int i = 0; i < CLUSTER_SLOTS; ++i) {
        info.add_slots(slots[i]);
    }
    info.set_numslots(numslots);
    info.set_numslaves(numslaves);
    info.set_ping_sent(ping_sent);
    info.set_pong_received(pong_received);
    info.set_fail_time(fail_time);
    info.set_voted_time(voted_time);
    info.set_repl_aof_name(repl_aof_name);
    info.set_repl_offset(repl_offset);
    info.set_repl_offset_time(repl_offset_time);
    if (isSlave() && slaveof){
        info.set_slave_of(slaveof->addr);
    }
}
