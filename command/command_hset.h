//
// Created by 王振奎 on 2020/10/6.
//

#ifndef NARUTO_COMMAND_HSET_H
#define NARUTO_COMMAND_HSET_H

#include "command.h"
#include "protocol/message.pb.h"

namespace naruto::command {

class CommandHset : public Command {
public:
    void exec(narutoClient *client) override;

    ~CommandHset() override = default;
};

}
#endif //NARUTO_COMMAND_HSET_H
