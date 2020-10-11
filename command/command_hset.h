//
// Created by 王振奎 on 2020/10/6.
//

#ifndef NARUTO_COMMAND_HSET_H
#define NARUTO_COMMAND_HSET_H

#include "command.h"
#include "protocol/message.pb.h"

namespace naruto{
namespace command {

class CommandHset : public Command {
public:
    void call(std::shared_ptr<database::Buckets> data, const utils::Bytes &request, utils::Bytes &response) override;

    ~CommandHset() override = default;
};

}
}
#endif //NARUTO_COMMAND_HSET_H
