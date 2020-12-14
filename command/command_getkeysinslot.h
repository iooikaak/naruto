//
// Created by kwins on 2020/12/9.
//

#ifndef NARUTO_COMMAND_GETKEYSINSLOT_H
#define NARUTO_COMMAND_GETKEYSINSLOT_H

#include "command.h"

#include "utils/bytes.h"

namespace naruto::command {
class CommandGetKeysinSlot : public Command{
public:
    void exec(void *client) override;
    ~CommandGetKeysinSlot() override = default;
};
}


#endif //NARUTO_COMMAND_GETKEYSINSLOT_H
