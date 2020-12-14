//
// Created by kwins on 2020/12/9.
//

#ifndef NARUTO_COMMAND_ADDSLOTS_H
#define NARUTO_COMMAND_ADDSLOTS_H

#include "command.h"

#include "utils/bytes.h"

namespace naruto::command {

class CommandAddslots : public Command{
public:
    void exec(void *client) override;
    ~CommandAddslots() override = default;
};

}

#endif //NARUTO_COMMAND_ADDSLOTS_H
