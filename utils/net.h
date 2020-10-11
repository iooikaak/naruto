//
// Created by 王振奎 on 2020/9/16.
//

#ifndef NARUTO_UTILS_NET_H
#define NARUTO_UTILS_NET_H

#include <list>
#include "client.h"

namespace naruto::utils{

class Net{
public:
    static int listen(int port, int tcp_backlog);
    static int setSocketReuseAddr(int fd);
    static void freeClient(std::list<narutoClient>);
};

}


#endif //NARUTO_UTILS_NET_H
