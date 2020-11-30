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

#include "client.h"
#include "global.h"
#include "utils/errors.h"
#include "utils/bytes.h"
#include "connection/connection.h"

// ConnectWorker 对应一个工作线程
// 每个线程会并行处理客户端 io
namespace naruto{

class ConnectWorker {
public:
    ConnectWorker();
    ~ConnectWorker();
    void run(int id);
    void clientFree(narutoClient* nc);
    void clientFree(std::shared_ptr<narutoClient>& nc);
    static void onAsync(ev::async& watcher, int events);
    static void onStopAsync(ev::async& watcher, int events);

    void stop();

    int tid;
    ev::dynamic_loop loop;
    // 用户触发新连接到来
    ev::async async_watcher;
    // 用于触发停止worker线程
    ev::async stop_async_watcher;
    std::deque<narutoClient*> conns;
    uint64_t conn_nums;
};

extern int worker_num;
extern ConnectWorker* workers;

}



#endif //NARUTO_CONNECT_WORKER_H
