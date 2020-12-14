//
// Created by 王振奎 on 2020/10/3.
//

#ifndef NARUTO_COMMAND_NF_H
#define NARUTO_COMMAND_NF_H

#include "command.h"

namespace naruto::command{

// 未知命令
class CommandNF : public Command {
public:

    void exec(void *client) override;

    ~CommandNF() override = default;
};

}



#endif //NARUTO_COMMAND_NF_H
