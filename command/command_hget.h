//
// Created by 王振奎 on 2020/10/3.
//

#ifndef NARUTO_COMMAND_HGET_H
#define NARUTO_COMMAND_HGET_H

#include "command.h"
#include "protocol/message.pb.h"

namespace naruto::command{

class CommandHget : public Command {
public:
    void exec(narutoClient *client) override;

    ~CommandHget() override = default;
};


}



#endif //NARUTO_COMMAND_HGET_H
