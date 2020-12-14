//
// Created by kwins on 2020/11/30.
//

#ifndef NARUTO_COMMAND_PONG_H
#define NARUTO_COMMAND_PONG_H

#include "command.h"

namespace naruto::command  {

class CommandPong : public Command {
public:
    void exec(void* link) override;
    ~CommandPong() override = default;
};

}

#endif //NARUTO_COMMAND_PONG_H
