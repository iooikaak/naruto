//
// Created by kwins on 2020/11/30.
//

#ifndef NARUTO_COMMAND_PONG_H
#define NARUTO_COMMAND_PONG_H

#include "command.h"

namespace naruto::command  {

class CommandPong : public Command {
public:
    void exec(narutoClient *client) override;
    void execMsg(uint16_t flag, uint16_t type, const unsigned char *msg, size_t n) override;
    ~CommandPong() override = default;
};

}

#endif //NARUTO_COMMAND_PONG_H
