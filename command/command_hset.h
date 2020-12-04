//
// Created by kwins on 2020/11/23.
//

#ifndef NARUTO_COMMAND_HSET_H
#define NARUTO_COMMAND_HSET_H

#include "command.h"

#include "utils/bytes.h"

namespace naruto::command {

class CommandHset : public Command{
public:
    void exec(narutoClient *client) override;
    client::command_reply execMsg(uint16_t,uint16_t,const unsigned char*, size_t) override;
    ~CommandHset() override = default;
};

}


#endif //NARUTO_COMMAND_HSET_H
