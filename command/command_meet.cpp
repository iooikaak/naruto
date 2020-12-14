//
// Created by kwins on 2020/12/9.
//

#include "command_meet.h"

#include "cluster/cluster_link.h"
#include "utils/pack.h"
#include "protocol/cluster_message.pb.h"
#include "protocol/client.pb.h"
#include "cluster/cluster.h"

void naruto::command::CommandMeet::exec(void *link) {
    auto client = static_cast<cluster::clusterLink*>(link);
    ::cluster::command_meet meet;
    auto type = utils::Pack::deSerialize(client->rbuf, meet);

    client::command_reply reply;
    if (meet.ip().empty() || meet.port() <= 0){
        reply.set_errcode(1);
        reply.set_errmsg("parameter not illegal");
        client->sendMsg(reply, type);
        return;
    }

    auto flag = (uint32_t)link::clientLink::flags::HANDSHAKE | (uint32_t)link::clientLink::flags::MEET;
    auto node = std::make_shared<cluster::clusterNode>(meet.ip(),meet.port(), flag);
    auto it = cluster::clsptr->view.nodes.find(node->addr);
    if (it != cluster::clsptr->view.nodes.end()){
        reply.set_errcode(1);
        reply.set_errmsg("node has been in meet");
        client->sendMsg(reply, type);
        return;
    }

    cluster::clsptr->view.nodes.emplace(node->addr,node);
    reply.set_errcode(0);
    client->sendMsg(reply, type);
}
