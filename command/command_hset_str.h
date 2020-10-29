//
// Created by 王振奎 on 2020/10/6.
//

#ifndef NARUTO_COMMAND_HSET_STR_H
#define NARUTO_COMMAND_HSET_STR_H

#include "command.h"

namespace naruto::command {

class CommandHsetStr : public Command {
public:
    void exec(narutoClient *client) override;

    ~CommandHsetStr() override = default;
};

}
#endif //NARUTO_COMMAND_HSET_STR_H
