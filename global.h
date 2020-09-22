//
// Created by 王振奎 on 2020/8/19.
//

#ifndef NARUTO_GLOBAL_H
#define NARUTO_GLOBAL_H

#include <mutex>
#include <condition_variable>
namespace naruto{

extern int init_success_workers;
extern int exit_success_workers;
// 控制多线程情况下，程序启动 和 关闭行为
extern std::mutex mux;
extern std::condition_variable cond;


extern std::string charset;

}

#endif //NARUTO_GLOBAL_H
