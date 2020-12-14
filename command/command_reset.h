//
// Created by kwins on 2020/12/9.
//

#ifndef NARUTO_COMMAND_RESET_H
#define NARUTO_COMMAND_RESET_H

#include "command.h"

#include "utils/bytes.h"

namespace naruto::command {
class CommandReset : public Command{
public:
    void exec(void *client) override;

    ~CommandReset() override = default;

};
}

#endif //NARUTO_COMMAND_RESET_H
