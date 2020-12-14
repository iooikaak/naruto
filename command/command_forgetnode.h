//
// Created by kwins on 2020/12/9.
//

#ifndef NARUTO_COMMAND_FORGETNODE_H
#define NARUTO_COMMAND_FORGETNODE_H

#include "command.h"

#include "utils/bytes.h"

namespace naruto::command {

class CommandForgetNode : public Command {
public:
    void exec(void *client) override;
    ~CommandForgetNode() override = default;
};

}


#endif //NARUTO_COMMAND_FORGETNODE_H
