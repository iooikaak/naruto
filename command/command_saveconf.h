//
// Created by kwins on 2020/12/9.
//

#ifndef NARUTO_COMMAND_SAVECONF_H
#define NARUTO_COMMAND_SAVECONF_H

#include "command.h"

#include "utils/bytes.h"

namespace naruto::command {
class CommandSaveconf : public Command{
public:
    void exec(void *client) override;

    ~CommandSaveconf() override = default;
};
}


#endif //NARUTO_COMMAND_SAVECONF_H
