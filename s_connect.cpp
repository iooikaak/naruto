//
// Created by 王振奎 on 2020/8/14.
//

#include "s_connect.h"

namespace naruto {

Connect::Connect(const naruto::net::ConnectOptions &ops, int fd) : naruto::net::Connector(ops, fd) {}


}

