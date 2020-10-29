//
// Created by kwins on 2020/10/29.
//

#ifndef NARUTO_COMMAND_HGET_INT_H
#define NARUTO_COMMAND_HGET_INT_H

#include "command.h"

namespace naruto::command{

class CommandHgetInt : public Command {
public:
    void exec(narutoClient *client) override;

    ~CommandHgetInt() override = default;
};


}


#endif //NARUTO_COMMAND_HGET_INT_H
