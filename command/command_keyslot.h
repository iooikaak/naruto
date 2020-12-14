//
// Created by kwins on 2020/12/9.
//

#ifndef NARUTO_COMMAND_KEYSLOT_H
#define NARUTO_COMMAND_KEYSLOT_H

#include "command.h"

#include "utils/bytes.h"

namespace naruto::command {
class CommandKeySlot : public Command {
public:
    void exec(void *client) override;

    ~CommandKeySlot() override = default;
};
}

#endif //NARUTO_COMMAND_KEYSLOT_H
