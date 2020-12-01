//
// Created by 王振奎 on 2020/8/14.
//

#ifndef NARUTO_CLIENTS_C_CONNECT_H
#define NARUTO_CLIENTS_C_CONNECT_H

#include "utils/bytes.h"

// 定义 FD 无效值
#define FD_INVALID (-1)

// 定义成员函数返回值
#define CONNECT_RT_OK 0
#define CONNECT_RT_ERR (-1)
#define CONNECT_RT_CLOSE (-2)

#define CONNECT_RETRIES 10
#define CONNECT_READ_BUF_SIZE (1024*16)

namespace naruto::connection{

class ConnectOptions{
public:
    ConnectOptions() = default;

    ConnectOptions(const ConnectOptions&) = default;
    ConnectOptions& operator=(const ConnectOptions&) = default;

    ConnectOptions(ConnectOptions&&) = default;
    ConnectOptions& operator=(ConnectOptions&&) = default;

    ~ConnectOptions() = default;

    std::string host;
    int port = 7290;

    std::chrono::milliseconds connect_timeout{ 100 };
    std::chrono::milliseconds read_timeout{ 200 };
    std::chrono::milliseconds write_timeout{ 200 };
};

class Connect {
public:
    enum class flags : unsigned {
        BLOCK = (1<<0),
        NO_BLOCK = (1<<1)
    };
    enum class status : unsigned {
        INIT = (1<<0),
        CONNECTED = (1<<1),
        CLOSE = (1<<2)
    };
public:
    explicit Connect(int fd);
    explicit Connect(ConnectOptions);

    Connect(const Connect&) = delete;
    Connect& operator= (const Connect&) = delete;

    Connect(Connect&&) = default;
    Connect& operator = (Connect&&) = default;
    ~Connect();
    int connect();

    void reset() noexcept;
    int send(const char*, size_t);
    void send(naruto::utils::Bytes&);
    void recv(naruto::utils::Bytes&);
    std::string remoteAddr() const;
    std::string addr() const; // 连接地址

    int fd() const;
    void setOps(const ConnectOptions&);

    bool broken() const noexcept;

    std::string errmsg() const;
    int errcode() const ;

    auto last_active() const -> std::chrono::time_point<std::chrono::steady_clock>;
    void update_last_active() noexcept;

    void reconnect();
    void close();

    friend void swap(Connect& lc, Connect& rc) noexcept;

private:

    int _set_blocking(bool blocking);

    int _wait_ready(long msec);

    int _check_connect_done(int* completed);

    int _check_socket_error();

    long _connect_timeout_msec();

    int _set_connect_timeout();

    void _set_error(int type, const std::string& prefix);
    int write_(naruto::utils::Bytes &pack);
    int read_(naruto::utils::Bytes &pack);

    struct sockaddr* addr_;
    size_t addrlen_;

    int err_;
    std::string errmsg_;

    int fd_;
    unsigned flag_{};
    status status_ {};
    char _ibuf[CONNECT_READ_BUF_SIZE]{};

    ConnectOptions opts_;
    std::chrono::steady_clock::time_point _last_active{};
};

}


#endif //NARUTO_CLIENTS_C_CONNECT_H
