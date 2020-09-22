//
// Created by 王振奎 on 2020/8/19.
//

#include "global.h"

namespace naruto{

int init_success_workers = 0;
int exit_success_workers = 0;
std::mutex mux = {};
std::condition_variable cond = {};

std::string charset = "0123456789abcdef";

}