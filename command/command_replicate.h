//
// Created by kwins on 2020/12/9.
//

#ifndef NARUTO_COMMAND_REPLICATE_H
#define NARUTO_COMMAND_REPLICATE_H

#include "command.h"

#include "utils/bytes.h"

namespace naruto::command {
class CommandReplicate : public Command {
public:
    void exec(void *client) override;

    ~CommandReplicate() override = default;
};
}


#endif //NARUTO_COMMAND_REPLICATE_H
