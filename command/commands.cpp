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
#include "command_aof.h"
#include "command_psync.h"
#include "command_pong.h"
#include "protocol/command_types.pb.h"

namespace naruto::command{

Commands::Commands() {
    std::cout << "init commands" << std::endl;
    // 初始化命令
    _cmd_nf = std::make_shared<CommandNF>();
    reg(cmdtype::Type::CLIENT_HSET, std::make_shared<CommandHset>());
    reg(cmdtype::Type::CLIENT_HGET, std::make_shared<CommandHget>());

    reg(cmdtype::Type::REPL_MULTI, std::make_shared<CommandAof>());
    reg(cmdtype::Type::REPL_PSYNC, std::make_shared<CommandPsync>());
    reg(cmdtype::Type::REPL_SLAVEOF, std::make_shared<CommandSlaveof>());
    reg(cmdtype::Type::REPL_PING, std::make_shared<CommandPing>());
    reg(cmdtype::Type::REPL_PONG, std::make_shared<CommandPong>());
}

void Commands::reg(int type, std::shared_ptr<Command> cmd) { _cmds.emplace(type, cmd); }

std::shared_ptr<Command> Commands::fetch(int type) {
    LOG(INFO) << "command fetch type:" << cmdtype::Type_descriptor()->FindValueByNumber(type)->name();
    auto it = _cmds.find(type);
    return it != _cmds.end() ? it->second : _cmd_nf;
}

std::shared_ptr<Commands> commands = std::make_shared<Commands>();

}







