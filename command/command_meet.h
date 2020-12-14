//
// Created by kwins on 2020/12/9.
//

#ifndef NARUTO_COMMAND_MEET_H
#define NARUTO_COMMAND_MEET_H

#include "command.h"

#include "utils/bytes.h"

namespace naruto::command {

/* 将给定地址的节点添加到当前节点所处的集群里面,在 cluster 轮询时会检查meet节点并进行处理 */
class CommandMeet : Command {
public:
private:
    void exec(void *client) override;
    ~CommandMeet() override = default;
};

}
#endif //NARUTO_COMMAND_MEET_H
