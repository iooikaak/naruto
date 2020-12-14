//
// Created by kwins on 2020/12/9.
//

#ifndef NARUTO_COMMAND_SETEPOCH_H
#define NARUTO_COMMAND_SETEPOCH_H

#include "command.h"

#include "utils/bytes.h"

namespace naruto::command {
class CommandSetEpoch : public Command {
public:
    void exec(void *client) override;

    ~CommandSetEpoch() override = default;

};
}

#endif //NARUTO_COMMAND_SETEPOCH_H
