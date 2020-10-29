//
// Created by 王振奎 on 2020/10/3.
//

#ifndef NARUTO_COMMANDS_H
#define NARUTO_COMMANDS_H

#include <unordered_map>
#include "command.h"
#include "utils/nocopy.h"
#include "command_nf.h"

namespace naruto::command {


class Commands : public utils::UnCopyable {
public:
    Commands();

    void reg(int type, std::shared_ptr<Command> cmd);

    std::shared_ptr <Command> fetch(int type);

private:


    std::shared_ptr<CommandNF> _cmd_nf;
    std::unordered_map<int, std::shared_ptr<Command>> _cmds;
};

}

#endif //NARUTO_COMMANDS_H
