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
#include <list>
#include <glog/logging.h>
#include "global.h"
#include "utils/errors.h"
#include "utils/bytes.h"
#include "connection/connection.h"
#include "link/client_link.h"

// ConnectWorker 对应一个工作线程
// 每个线程会并行处理客户端 io
namespace naruto{

class ConnectWorker {
public:
    explicit ConnectWorker();
    void run(int id);
    void stop();
    ~ConnectWorker();

    static void onAsync(ev::async& watcher, int events);
    static void onStopAsync(ev::async& watcher, int events);

    int tid;
    ev::dynamic_loop loop;
    // 用户触发新连接到来
    ev::async async;
    // 用于触发停止worker线程
    ev::async stop_async;
    std::deque<link::clientLink*> conns;
    uint64_t conn_nums;
};

extern int worker_num;
extern ConnectWorker* workers;

}



#endif //NARUTO_CONNECT_WORKER_H
