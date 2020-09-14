//
// Created by 王振奎 on 2020/8/14.
//

#ifndef NARUTO_CLIENTS_C_CONNECT_H
#define NARUTO_CLIENTS_C_CONNECT_H

#include "net/connector.h"

namespace naruto{
namespace client{

class Connect {
public:
    explicit Connect(const naruto::net::ConnectOptions&);

    Connect(const Connect&) = delete;
    Connect& operator= (const Connect&) = delete;

    Connect(Connect&&) = default;
    Connect& operator = (Connect&&) = default;

    int connect();

    auto last_active() const -> std::chrono::time_point<std::chrono::steady_clock>;
    void update_last_active() noexcept;

    void reconnect();

    friend void swap(Connect& lc, Connect& rc) noexcept;

    using ConnectorPtr = std::unique_ptr<naruto::net::Connector>;
    ConnectorPtr connector;

private:

    timeval _to_timeval(const std::chrono::milliseconds& duration) const;

    int _set_blocking(bool blocking);

    int _wait_ready(long msec);

    int _check_connect_done(int* completed);

    int _check_socket_error();

    long _connect_timeout_msec();

    int _set_connect_timeout();

    std::chrono::time_point<std::chrono::steady_clock> _last_active{};
};

}
}


#endif //NARUTO_CLIENTS_C_CONNECT_H
