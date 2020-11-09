//
// Created by 王振奎 on 2020/10/6.
//

#ifndef NARUTO_COMMAND_HSET_STR_H
#define NARUTO_COMMAND_HSET_STR_H

#include "command.h"

#include "database/buckets.h"
#include "client.h"
#include "database/string_.h"
#include "utils/pack.h"
#include "protocol/client.pb.h"

namespace naruto::command {

class CommandHsetStr : public Command {
public:
    void exec(narutoClient *client) override;

    ~CommandHsetStr() override = default;
};

}
#endif //NARUTO_COMMAND_HSET_STR_H
