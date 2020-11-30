//
// Created by kwins on 2020/11/26.
//

#ifndef NARUTO_COMMAND_MULTI_H
#define NARUTO_COMMAND_MULTI_H

#include "command.h"

#include "utils/bytes.h"

namespace naruto::command {

class CommandMulti : public Command{
public:
    void exec(narutoClient *client) override;

    void execMsg(uint16_t flag, uint16_t type, const unsigned char *msg, size_t n) override;

    ~CommandMulti() override = default;
};

}
#endif //NARUTO_COMMAND_MULTI_H
