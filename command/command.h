//
// Created by 王振奎 on 2020/10/2.
//

#ifndef NARUTO_COMMAND_H
#define NARUTO_COMMAND_H

namespace naruto{

class narutoClient;

namespace command {

class Command {
public:
    virtual void exec(narutoClient* client) = 0;
    virtual ~Command() = default;
};

}
}


#endif //NARUTO_COMMAND_H
