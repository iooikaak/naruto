//
// Created by 王振奎 on 2020/11/1.
//

#include "command_psync.h"

#include "client.h"
#include "protocol/replication.pb.h"
#include "utils/pack.h"

void naruto::command::CommandPsync::exec(naruto::narutoClient *client) {
    replication::command_psync psync;
    utils::Pack::deSerialize(client->rbuf, psync);
}

void naruto::command::CommandPsync::execMsg(uint16_t flag, uint16_t type, const unsigned char *msg, size_t n) {

}
