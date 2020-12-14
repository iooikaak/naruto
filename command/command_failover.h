//
// Created by kwins on 2020/12/9.
//

#ifndef NARUTO_COMMAND_FAILOVER_H
#define NARUTO_COMMAND_FAILOVER_H

#include "command.h"

#include "utils/bytes.h"

namespace naruto::command {
class CommandFailover : public Command {
public:
    void exec(void *client) override;

    ~CommandFailover() override = default;
};
}

#endif //NARUTO_COMMAND_FAILOVER_H
