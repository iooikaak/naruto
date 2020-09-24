//
// Created by 王振奎 on 2020/8/19.
//

#ifndef NARUTO_CONNECT_WORKER_H
#define NARUTO_CONNECT_WORKER_H

#include <thread>
#include <ev++.h>
#include <deque>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "global.h"
#include "s_connect.h"
#include "utils/bytes.h"

// ConnectWorker 对应一个工作线程
// 每个线程会并行处理客户端 io
namespace naruto{



struct connect{
    int sdf;
    char saddr[16];
    int port;
    connect* next;
};

class ConnectWorker {
public:
    ConnectWorker();
    ~ConnectWorker();

    void run(int id);
    void onEvents(ev::io& watcher, int events);
    static void onAsync(ev::async& watcher, int events);
    static void onStopAsync(ev::async& watcher, int events);
    void stop();

    int tid;
    ev::dynamic_loop loop;
    // 用户触发新连接到来
    ev::async async_watcher;
    // 用于触发停止worker线程
    ev::async stop_async_watcher;
    std::deque<Connect*> conns;

private:
    void _close_connect(ev::io& watcher, Connect* c);
    naruto::utils::Bytes _aof_buf;
};

}



#endif //NARUTO_CONNECT_WORKER_H
