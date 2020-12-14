//
// Created by kwins on 2020/12/9.
//

#ifndef NARUTO_COMMAND_COUNTKEYINSLOT_H
#define NARUTO_COMMAND_COUNTKEYINSLOT_H

#include "command.h"

#include "utils/bytes.h"

namespace naruto::command {
class CommandCountKeyinSlot : public Command {
public:
    void exec(void *client) override;

    ~CommandCountKeyinSlot() override = default;
};
}

#endif //NARUTO_COMMAND_COUNTKEYINSLOT_H
