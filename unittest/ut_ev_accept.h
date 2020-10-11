//
// Created by 王振奎 on 2020/8/10.
//

#ifndef NARUTO_UNITTEST_UT_EV_ACCEPT_H
#define NARUTO_UNITTEST_UT_EV_ACCEPT_H
#include <gtest/gtest.h>
#include <ev.h>
#include <netinet/in.h>
#include <iostream>
#include <glog/logging.h>
#include <ev++.h>
#include <vector>
#include <thread>
#include <mutex>
#include <deque>
#include <cstdlib>
#include <memory>
#include <cstring>
#include <arpa/inet.h>
#include <condition_variable>

#define  PORT_NO 7290
#define BUFFER_SIZE 10

struct connQueueItem {
    int sdf;
    char saddr[16];
    int port;
    connQueueItem *next;
};

struct workThread {
    std::thread::id tid;
    struct ev_loop *loop;
    struct ev_async async_watcher;
    std::deque<connQueueItem *> *new_conn_queue;
};

struct dispatcherThread {
    std::thread::id tid;
    struct ev_loop *loop;
    struct ev_io accept_watcher;
};

static dispatcherThread dispatcher_thread;
static workThread *work_thread;

static int init_cout = 0;
static std::mutex init_lock;
static std::condition_variable init_cond;
static int round_robin = 0;

static void *worker_libev(void *arg) {
    LOG(INFO) << "worker_libev...";
    workThread *me = static_cast<workThread *>(arg);
    std::unique_lock<std::mutex> lck(init_lock);
    init_cout++;
    init_cond.notify_one();
    lck.unlock();
    me->tid = std::this_thread::get_id();
    ev_loop(me->loop, 0);
    return nullptr;
}

static void read_cb(struct ev_loop *loop, ev_io *read_watcher, int events) {
    LOG(INFO) << "read_cb...";

    char buffer[BUFFER_SIZE];
    if (EV_ERROR & events) {
        LOG(ERROR) << "read event error";
        return;
    }

    ssize_t readlen;
    LOG(INFO) << "read_cb recv wait, watcher fd:" << read_watcher->fd;
    if ((readlen = recv(read_watcher->fd, buffer, BUFFER_SIZE, MSG_WAITALL)) < 0) {
        LOG(ERROR) << "recv error";
        return;
    }

    if (readlen == 0) {
        ev_io_stop(loop, read_watcher);
        free(read_watcher);
        LOG(ERROR) << "peer might closing";
        return;
    }
    LOG(INFO) << "read client data:" << buffer;
    bzero(buffer, readlen);
}

static void async_cb(EV_P_ ev_async *w, int events) {
    LOG(INFO) << "async_cb...";
    connQueueItem *item;
    workThread *wt = static_cast<workThread *>(w->data);
    auto conn_q = wt->new_conn_queue;

    LOG(INFO) << "async_cb new_conn_queue size:" << wt->new_conn_queue->size();

    item = conn_q->front();
    conn_q->pop_front();

    if (item != nullptr) {
        LOG(INFO) << "async_cb item...";
        struct ev_io *recv_watcher = (struct ev_io *) malloc(sizeof(ev_io));
        ev_io_init(recv_watcher, read_cb, item->sdf, EV_READ);
        ev_io_start(wt->loop, recv_watcher);

        free(item);
        item = nullptr;
    } else {
        LOG(INFO) << "async_cb nullptr...";
    }
}

static void setup_work_thread(workThread *me) {
    me->loop = ev_loop_new(0);
    if (!me->loop) {
        LOG(ERROR) << "setup_work_thread oom";
        exit(1);
    }
    me->async_watcher.data = me;
    me->new_conn_queue = new std::deque<connQueueItem *>(0);

    ev_async_init(&me->async_watcher, async_cb);
    ev_async_start(me->loop, &me->async_watcher);
}

void thread_init() {
    int nthreads = 4;
    dispatcher_thread.loop = ev_default_loop(0);
    dispatcher_thread.tid = std::this_thread::get_id();
    LOG(INFO) << "thread_init....";
    work_thread = (workThread *) calloc(nthreads, sizeof(workThread));
    for (int i = 0; i < nthreads; ++i) {
        setup_work_thread(&work_thread[i]);
    }
    for (int j = 0; j < nthreads; ++j) {
        std::thread([j]() {
            worker_libev(&work_thread[j]);
        }).detach();
    }

    std::unique_lock<std::mutex> lck(init_lock);
    while (init_cout < nthreads) {
        LOG(INFO) << "wait..." << init_cout;
        init_cond.wait(lck);
    }
    LOG(INFO) << "thread_init end....";
}

void dispatch_conn(int anewfd, struct sockaddr_in asin) {
    connQueueItem *new_item = (connQueueItem *) calloc(1, sizeof(connQueueItem));
    if (new_item == nullptr) {
        LOG(ERROR) << "dispatch_conn oom";
        exit(1);
    }

    new_item->sdf = anewfd;
    new_item->port = asin.sin_port;
    strcpy(new_item->saddr, (char *) inet_ntoa(asin.sin_addr));

    int robin = round_robin % init_cout;
    work_thread[robin].new_conn_queue->push_back(new_item);
    ev_async_send(work_thread[robin].loop, &(work_thread[robin].async_watcher));
    round_robin++;
}

class TestLibevAccept : public ::testing::Test {
public:
    // Sets up the test fixture.
    virtual void SetUp() {
        _loop = ev_default_loop(0);
    }

    static void accept_cb(struct ev_loop *loop, struct ev_io *accept_watcher, int events) {
        LOG(INFO) << "accept_cb...";
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_sd;
        ev_io *read_ev = (ev_io *) malloc(sizeof(ev_io));
        if (EV_ERROR & events) {
            LOG(ERROR) << "accept event error";
            return;
        }

        if ((client_sd = accept(accept_watcher->fd, (sockaddr *) &client_addr, &client_len)) < 0) {
            LOG(ERROR) << "accept error";
            return;
        }
        LOG(INFO) << "accept_cb dispatch conn:" << client_sd;
        dispatch_conn(client_sd, client_addr);
    }

    struct ev_loop *_loop;
};


TEST_F(TestLibevAccept, libev_accept) {
    LOG(INFO) << "thread_init...";
    thread_init();
    LOG(INFO) << "thread_init end...";

    struct sockaddr_in addr;
    int addr_len = sizeof(addr);
    int sd;

    if ((sd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        LOG(ERROR) << "socket error";
        return;
    }
    LOG(INFO) << "bind sd:" << sd;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT_NO);
    addr.sin_addr.s_addr = INADDR_ANY;
    int sin_size;

    if (bind(sd, (const struct sockaddr *) &addr, addr_len) != 0) {
        char buf[128];
        strerror_r(errno, buf, sizeof(buf));
        LOG(ERROR) << "bind error:" << buf;
        return;
    }

    if (listen(sd, 1000) < 0) {
        LOG(ERROR) << "listen error";
        return;
    }

    ev_io_init(&(dispatcher_thread.accept_watcher), accept_cb, sd, EV_READ);
    ev_io_start(dispatcher_thread.loop, &(dispatcher_thread.accept_watcher));
    ev_loop(dispatcher_thread.loop, 0);

    ev_loop_destroy(dispatcher_thread.loop);
}

#endif //NARUTO_UNITTEST_UT_EV_ACCEPT_H
