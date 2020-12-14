//
// Created by kwins on 2020/12/9.
//

#ifndef NARUTO_COMMAND_IMPORTING_H
#define NARUTO_COMMAND_IMPORTING_H

#include "command.h"

#include "utils/bytes.h"

namespace naruto::command {

class CommandImporting : public Command {
public:
    void exec(void *client) override;
    ~CommandImporting() override = default;
};

}

#endif //NARUTO_COMMAND_IMPORTING_H
