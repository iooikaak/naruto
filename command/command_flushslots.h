//
// Created by kwins on 2020/12/9.
//

#ifndef NARUTO_COMMAND_FLUSHSLOTS_H
#define NARUTO_COMMAND_FLUSHSLOTS_H

#include "command.h"

#include "utils/bytes.h"

namespace naruto::command {

class CommandFlushSlots : public Command {
public:
    void exec(void *client) override;
    ~CommandFlushSlots() override = default;
};

}

#endif //NARUTO_COMMAND_FLUSHSLOTS_H
