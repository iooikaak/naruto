//
// Created by 王振奎 on 2020/10/15.
//

#ifndef NARUTO_COMMAND_SLAVEOF_H
#define NARUTO_COMMAND_SLAVEOF_H

#include "command.h"

namespace naruto::command {

class CommandSlaveof : public Command {
public:
    void exec(narutoClient *client) override;

    void execMsg(uint16_t flag, uint16_t type, const unsigned char *msg, size_t n) override;

    ~CommandSlaveof() override = default;
};

}

#endif //NARUTO_COMMAND_SLAVEOF_H
