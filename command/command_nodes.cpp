//
// Created by kwins on 2020/12/9.
//

#include "command_nodes.h"
#include "protocol/cluster_message.pb.h"
#include "utils/pack.h"
#include "cluster/cluster_link.h"
#include "cluster/cluster.h"
#include "protocol/command_types.pb.h"

void naruto::command::CommandNodes::exec(void *link) {
    auto client = static_cast<cluster::clusterLink*>(link);
    ::cluster::command_nodes request;
    auto type = utils::Pack::deSerialize(client->rbuf, request);
    ::cluster::command_nodes_reply reply;
    if (type != cmdtype::Type::CLUSTER_INFO_NODES){
        reply.mutable_state()->set_errcode(1);
        reply.mutable_state()->set_errmsg("message type error");
        client->sendMsg(reply, cmdtype::Type::CLUSTER_INFO_NODES);
        return;
    }

    for (const auto& v : cluster::clsptr->view.nodes){
        if (v.second->flags & request.filter_flags()) continue;
        auto info = reply.add_nodes();
        v.second->desc(*info);
        // mysql 节点数据
        if (v.second->isMyself()){
            for (int i = 0; i < CLUSTER_SLOTS; ++i) {
                if (cluster::clsptr->view.migrating_slots_to[i] && cluster::clsptr->view.slots[i]){
                    auto mign = cluster::clsptr->view.slots[i];
                    info->add_migrating_slots_to(std::to_string(i) +"->-" + mign->addr);
                } else if (cluster::clsptr->view.importing_slots_from[i] && cluster::clsptr->view.slots[i]){
                    auto impn = cluster::clsptr->view.slots[i];
                    info->add_importing_slots_from(std::to_string(i) +"-<-" + impn->addr);
                }
            }
        }
    }
    client->sendMsg(reply, type);
}
