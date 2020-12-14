//
// Created by kwins on 2020/12/9.
//

#ifndef NARUTO_COMMAND_CLUSTERINFO_H
#define NARUTO_COMMAND_CLUSTERINFO_H

#include "command.h"

#include "utils/bytes.h"

namespace naruto::command {

class CommandClusterinfo : public Command{
public:
    void exec(void *client) override;
    ~CommandClusterinfo() override = default;
};

}

#endif //NARUTO_COMMAND_CLUSTERINFO_H
