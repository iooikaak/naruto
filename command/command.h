//
// Created by 王振奎 on 2020/10/2.
//

#ifndef NARUTO_COMMAND_H
#define NARUTO_COMMAND_H

#include "utils/bytes.h"
#include "utils/nocopy.h"
#include "database/buckets.h"

#define COMMAND_NOT_FOUND 0

#define COMMAND_REPL_PING 1
#define COMMAND_REPL_PONG 2
#define COMMAND_REPL_ACK 3
#define COMMAND_REPL_PSYNC 4
#define COMMAND_REPL_FULLSYNC 5
#define COMMAND_REPL_PARTSYNC 6

#define COMMAND_CLIENT_HGET 1
#define COMMAND_CLIENT_HSET 2

namespace naruto{
namespace command {

class Command {
public:
    // src 请求数据
    virtual void call(std::shared_ptr<database::Buckets> data, const utils::Bytes &request, utils::Bytes &response) = 0;

    virtual ~Command() = default;
};

}
}



#endif //NARUTO_COMMAND_H
