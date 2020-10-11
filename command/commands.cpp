//
// Created by 王振奎 on 2020/10/3.
//

#include "commands.h"

naruto::command::Commands::Commands() {
    std::cout << "init commands" << std::endl;
    // 初始化命令
    _cmd_nf = std::make_shared<CommandNF>();
    reg(COMMAND_CLIENT_HGET, std::make_shared<CommandHget>());
    reg(COMMAND_CLIENT_HSET, std::make_shared<CommandHset>());
}

void naruto::command::Commands::reg(int type, std::shared_ptr<Command> cmd) { _cmds.emplace(type, cmd); }


std::shared_ptr<naruto::command::Command> naruto::command::Commands::fetch(int type) {
    LOG(INFO) << "command fetch type:" << type;
    auto it = _cmds.find(type);
    return it != _cmds.end() ? it->second : _cmd_nf;
}
