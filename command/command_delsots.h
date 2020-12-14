//
// Created by kwins on 2020/12/9.
//

#ifndef NARUTO_COMMAND_DELSOTS_H
#define NARUTO_COMMAND_DELSOTS_H

#include "command.h"

#include "utils/bytes.h"

namespace naruto::command {
class CommandDelSots : public Command {
public:
    void exec(void *client) override;

    ~CommandDelSots() override = default;
};
}

#endif //NARUTO_COMMAND_DELSOTS_H
