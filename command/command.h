//
// Created by 王振奎 on 2020/10/2.
//

#ifndef NARUTO_COMMAND_H
#define NARUTO_COMMAND_H

#include <google/protobuf/message.h>
#include "protocol/client.pb.h"

namespace naruto::command {

class Command {
public:
    virtual void exec(void* client) = 0;
    virtual client::command_reply execMsg(uint16_t flag, uint16_t type, const unsigned char* msg, size_t n){
        return client::command_reply{};
    };
    virtual ~Command() = default;
};

}


#endif //NARUTO_COMMAND_H
