//
// Created by 王振奎 on 2020/10/3.
//

#ifndef NARUTO_COMMAND_HGET_STR_H
#define NARUTO_COMMAND_HGET_STR_H

#include "command.h"
#include "database/buckets.h"

namespace naruto::command{

class CommandHgetStr : public Command {
public:
    void exec(narutoClient *client) override;

    ~CommandHgetStr() override = default;
};


}



#endif //NARUTO_COMMAND_HGET_STR_H
