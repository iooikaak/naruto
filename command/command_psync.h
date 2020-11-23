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

    void execMsg(uint16_t flag, uint16_t type, const unsigned char *msg, size_t n) override;

    ~CommandPsync() override = default;
};

}

#endif //NARUTO_COMMAND_PSYNC_H
