//
// Created by 王振奎 on 2020/10/3.
//

#include "commands.h"

#include <glog/logging.h>
#include "command_nf.h"
#include "command_hset_str.h"
#include "command_hset_int.h"
#include "command_hget_str.h"
#include "command_hget_int.h"
#include "command_slaveof.h"
#include "command_ping.h"
#include "protocol/client.pb.h"
#include "protocol/replication.pb.h"

namespace naruto::command{

Commands::Commands() {
    std::cout << "init commands" << std::endl;
    // 初始化命令
    _cmd_nf = std::make_shared<CommandNF>();
    reg(client::HGET_STRING, std::make_shared<CommandHgetStr>());
    reg(client::HGET_INT, std::make_shared<CommandHgetInt>());

    reg(client::HSET_STRING, std::make_shared<CommandHsetStr>());
    reg(client::HSET_INT, std::make_shared<CommandHsetInt>());

    reg(replication::SLAVEOF, std::make_shared<CommandSlaveof>());
    reg(replication::PING, std::make_shared<CommandPing>());
}

void Commands::reg(int type, std::shared_ptr<Command> cmd) { _cmds.emplace(type, cmd); }

std::shared_ptr<Command> Commands::fetch(int type) {
    LOG(INFO) << "command fetch type:" << type;
    auto it = _cmds.find(type);
    return it != _cmds.end() ? it->second : _cmd_nf;
}

std::shared_ptr<Commands> commands = std::make_shared<Commands>();

}







