//
// Created by 王振奎 on 2020/10/3.
//

#include "commands.h"

#include <glog/logging.h>
#include "command_nf.h"
#include "command_hset.h"
#include "command_hget.h"
#include "command_slaveof.h"
#include "command_ping.h"
#include "command_multi.h"
#include "command_psync.h"
#include "command_pong.h"
#include "protocol/command_types.pb.h"

namespace naruto::command{

Commands::Commands() {
    std::cout << "init commands" << std::endl;
    // 初始化命令
    _cmd_nf = std::make_shared<CommandNF>();
    reg(client::Type::HSET, std::make_shared<CommandHset>());
    reg(client::Type::HGET, std::make_shared<CommandHget>());

    reg(client::Type::MULTI, std::make_shared<CommandMulti>());
    reg(client::Type::PSYNC, std::make_shared<CommandPsync>());
    reg(client::Type::SLAVEOF, std::make_shared<CommandSlaveof>());
    reg(client::Type::PING, std::make_shared<CommandPing>());
    reg(client::Type::PONG, std::make_shared<CommandPong>());
}

void Commands::reg(int type, std::shared_ptr<Command> cmd) { _cmds.emplace(type, cmd); }

std::shared_ptr<Command> Commands::fetch(int type) {
    LOG(INFO) << "command fetch type:" << type;
    auto it = _cmds.find(type);
    return it != _cmds.end() ? it->second : _cmd_nf;
}

std::shared_ptr<Commands> commands = std::make_shared<Commands>();

}







