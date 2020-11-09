//
// Created by 王振奎 on 2020/10/24.
//

#ifndef NARUTO_COMMAND_PING_H
#define NARUTO_COMMAND_PING_H

#include "command.h"

namespace naruto::command {

class CommandPing : public Command {
public:
    void exec(narutoClient *client) override;
    ~CommandPing() override = default;
};

}

#endif //NARUTO_COMMAND_PING_H
