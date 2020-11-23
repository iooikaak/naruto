//
// Created by kwins on 2020/11/23.
//

#ifndef NARUTO_COMMAND_HGET_H
#define NARUTO_COMMAND_HGET_H

#include "command.h"

namespace naruto::command {

class CommandHget : public Command {
public:
    void exec(narutoClient *client) override;

    void execMsg(uint16_t flag, uint16_t type, const unsigned char *msg, size_t n) override;

    ~CommandHget() override = default;
};

}


#endif //NARUTO_COMMAND_HGET_H
