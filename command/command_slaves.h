//
// Created by kwins on 2020/12/9.
//

#ifndef NARUTO_COMMAND_SLAVES_H
#define NARUTO_COMMAND_SLAVES_H

#include "command.h"

#include "utils/bytes.h"

namespace naruto::command {
class CommandSlaves : public Command {
public:
    void exec(void *client) override;

    ~CommandSlaves() override = default;
};
}

#endif //NARUTO_COMMAND_SLAVES_H
