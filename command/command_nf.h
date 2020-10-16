//
// Created by 王振奎 on 2020/10/3.
//

#ifndef NARUTO_COMMAND_NF_H
#define NARUTO_COMMAND_NF_H

#include "command.h"
#include "protocol/message.pb.h"

namespace naruto::command{

// 未知命令
class CommandNF : public Command {
public:
    void call(std::shared_ptr<database::Buckets> data, const utils::Bytes &request, utils::Bytes &response) override;

    ~CommandNF() override = default;
};

}



#endif //NARUTO_COMMAND_NF_H
