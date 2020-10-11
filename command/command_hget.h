//
// Created by 王振奎 on 2020/10/3.
//

#ifndef NARUTO_COMMAND_HGET_H
#define NARUTO_COMMAND_HGET_H

#include "command.h"
#include "protocol/message.pb.h"

namespace naruto{
namespace command{

class CommandHget : public Command {
public:
    void call(std::shared_ptr<database::Buckets> data, const utils::Bytes &request, utils::Bytes &response) override;

    ~CommandHget() override = default;
};


}
}



#endif //NARUTO_COMMAND_HGET_H
