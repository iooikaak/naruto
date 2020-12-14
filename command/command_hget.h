//
// Created by kwins on 2020/11/23.
//

#ifndef NARUTO_COMMAND_HGET_H
#define NARUTO_COMMAND_HGET_H

#include "command.h"

namespace naruto::command {

class CommandHget : public Command {
public:
    void exec(void *link) override;
    ~CommandHget() override = default;
};

}


#endif //NARUTO_COMMAND_HGET_H
