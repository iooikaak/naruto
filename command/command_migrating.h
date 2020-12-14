//
// Created by kwins on 2020/12/9.
//

#ifndef NARUTO_COMMAND_MIGRATING_H
#define NARUTO_COMMAND_MIGRATING_H

#include "command.h"

#include "utils/bytes.h"

namespace naruto::command {
class CommandMigrating : public Command {
public:
    void exec(void *client) override;

    ~CommandMigrating() override = default;
};
}

#endif //NARUTO_COMMAND_MIGRATING_H
