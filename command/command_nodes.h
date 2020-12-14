//
// Created by kwins on 2020/12/9.
//

#ifndef NARUTO_COMMAND_NODES_H
#define NARUTO_COMMAND_NODES_H

#include "command.h"

#include "utils/bytes.h"

namespace naruto::command {

// 列出集群所有节点的信息
class CommandNodes : public Command {
public:
    void exec(void *client) override;
    ~CommandNodes() override = default;
};

}


#endif //NARUTO_COMMAND_NODES_H
