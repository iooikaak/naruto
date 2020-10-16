//
// Created by 王振奎 on 2020/10/15.
//

#ifndef NARUTO_COMMAND_SLAVEOF_H
#define NARUTO_COMMAND_SLAVEOF_H

#include "command.h"
#include "protocol/message.pb.h"

namespace naruto::command {

class CommandSlaveof : public Command {
public:
    void call(std::shared_ptr<database::Buckets> data, const utils::Bytes &request, utils::Bytes &response) override;

    ~CommandSlaveof() override = default;
};

}

#endif //NARUTO_COMMAND_SLAVEOF_H
