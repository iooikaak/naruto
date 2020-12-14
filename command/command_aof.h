//
// Created by kwins on 2020/11/26.
//

#ifndef NARUTO_COMMAND_AOF_H
#define NARUTO_COMMAND_AOF_H

#include "command.h"

#include "utils/bytes.h"

namespace naruto::command {

class CommandAof : public Command{
public:
    void exec(void *link) override;

    client::command_reply execMsg(uint16_t flag, uint16_t type, const unsigned char *msg, size_t n) override;

    ~CommandAof() override = default;
};

}
#endif //NARUTO_COMMAND_AOF_H
