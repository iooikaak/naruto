//
// Created by 王振奎 on 2020/10/3.
//

#ifndef NARUTO_COMMANDS_H
#define NARUTO_COMMANDS_H

#include "command.h"
#include "command_nf.h"
#include "command_hget.h"
#include "command_hset.h"

namespace naruto::command {


class Commands : public utils::UnCopyable {
public:
    Commands();
//    static Commands& instance(){
//        static Commands commands;
//        return commands;
//    }

    void reg(int type, std::shared_ptr<Command> cmd);

    std::shared_ptr <Command> fetch(int type);

private:


    std::shared_ptr<CommandNF> _cmd_nf;
    std::unordered_map<int, std::shared_ptr<Command>> _cmds;
};

}

#endif //NARUTO_COMMANDS_H
