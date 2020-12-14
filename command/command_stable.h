//
// Created by kwins on 2020/12/9.
//

#ifndef NARUTO_COMMAND_STABLE_H
#define NARUTO_COMMAND_STABLE_H

#include "command.h"

#include "utils/bytes.h"

namespace naruto::command {
class CommandStable : public Command {
public:
    void exec(void *client) override;

    ~CommandStable() override = default;
};

}
#endif //NARUTO_COMMAND_STABLE_H
