//
// Created by kwins on 2020/10/29.
//

#ifndef NARUTO_COMMAND_HSET_INT_H
#define NARUTO_COMMAND_HSET_INT_H

#include "command.h"
#include "database/buckets.h"

namespace naruto::command {

class CommandHsetInt : public Command{
public:
    void exec(narutoClient *client) override;
    ~CommandHsetInt() override = default;
};

}

#endif //NARUTO_COMMAND_HSET_INT_H
