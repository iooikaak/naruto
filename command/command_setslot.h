//
// Created by kwins on 2020/12/9.
//

#ifndef NARUTO_COMMAND_SETSLOT_H
#define NARUTO_COMMAND_SETSLOT_H

#include "command.h"

#include "utils/bytes.h"

namespace naruto::command {
class CommandSetSlot : public Command{
public:
    void exec(void *client) override;

    ~CommandSetSlot() override = default;

};

}
#endif //NARUTO_COMMAND_SETSLOT_H
