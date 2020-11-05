//
// Created by 王振奎 on 2020/11/1.
//

#ifndef NARUTO_COMMAND_PSYNC_H
#define NARUTO_COMMAND_PSYNC_H

#include "command.h"

namespace naruto::command {

class CommandPsync : public Command {
public:
    void exec(narutoClient *client) override;
    ~CommandPsync() = default;
};

}

#endif //NARUTO_COMMAND_PSYNC_H
