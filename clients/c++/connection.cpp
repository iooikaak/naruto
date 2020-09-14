//
// Created by 王振奎 on 2020/8/14.
//


#include <fcntl.h>
#include <cstring>
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <unistd.h>
#include <poll.h>
#include <netdb.h>
#include <sys/socket.h>
#include <cassert>
#include <glog/logging.h>


#include "utils/errors.h"
#include "connection.h"

namespace naruto{
namespace client{

Connect::Connect(const naruto::net::ConnectOptions & opts) : connector(new naruto::net::Connector(opts)) {}

int Connect::connect() {
    int client_fd;
    struct sockaddr_in addr{};
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(connector->_options.host.c_str());
    LOG(INFO) << "connect port:" << connector->_options.port;
    addr.sin_port = htons(connector->_options.port);

    int blocking = (connector->_flags & CONNECT_FLAGS_BLOCK);
    int retry_times = 0;

    if ((client_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        connector->_set_error(CONNECT_ERROR_OTHER, "socket");
        return CONNECT_RT_ERR;
    }

retry:
    if (::connect(client_fd, (sockaddr*)&addr, sizeof(addr)) < 0){
        if (errno == EHOSTUNREACH){
            connector->closeConnect();
            return CONNECT_RT_ERR;
        }else if (errno == EINPROGRESS){
            if (blocking){
                _wait_ready(_connect_timeout_msec());
            }
        }else if (errno == EADDRNOTAVAIL){
            if (++retry_times >= CONNECT_RETRIES){
                connector->_set_error(CONNECT_ERROR_IO, "retry max times");
                return CONNECT_RT_ERR;
            }else{
                connector->closeConnect();
                goto retry;
            }
        }

        connector->_set_error(CONNECT_ERROR_OTHER, "::connect");
        return CONNECT_RT_ERR;
    }
    connector->_fd = client_fd;
    if (_set_blocking(0) != CONNECT_RT_OK){
        connector->_set_error(CONNECT_ERROR_OTHER, "_set_blocking");
        return CONNECT_RT_ERR;
    }

    connector->_flags |= CONNECT_FLAGS_CONNECTED;
    connector->_addr = (struct sockaddr*) malloc(sizeof(struct sockaddr));
    memcpy(connector->_addr, (sockaddr*)&addr, sizeof(struct sockaddr));
    return _set_connect_timeout();
}

// 重连
void Connect::reconnect(){
    Connect conn(connector->_options);

    if (conn.connect() != CONNECT_RT_OK)
        naruto::utils::throw_err(connector->_err, connector->_errmsg);

    assert(!conn.connector->broken());

    swap(*this, conn);
}

auto Connect::last_active() const ->
    std::chrono::time_point<std::chrono::steady_clock> { return _last_active; }

void Connect::update_last_active() noexcept { _last_active = std::chrono::steady_clock::now(); }

void swap(Connect &lc, Connect &rc) noexcept {
    std::swap(lc.connector, rc.connector);
    std::swap(lc._last_active, rc._last_active);
}

int Connect::_set_connect_timeout(){
    timeval read_timeout = _to_timeval(connector->_options.read_timeout);
    timeval write_timeout = _to_timeval(connector->_options.write_timeout);
    if (setsockopt(connector->_fd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof(read_timeout))){
        connector->_set_error(CONNECT_ERROR_IO,"setsockopt(SO_RCVTIMEO)");
        return CONNECT_RT_ERR;
    }
    if (setsockopt(connector->_fd, SOL_SOCKET, SO_SNDTIMEO, &write_timeout, sizeof(write_timeout))){
        connector->_set_error(CONNECT_ERROR_IO,"setsockopt(SO_SNDTIMEO)");
        return CONNECT_RT_ERR;
    }
    return CONNECT_RT_OK;
}

long Connect::_connect_timeout_msec(){
    long msec = std::chrono::duration_cast<std::chrono::microseconds>(connector->_options.connect_timeout).count();
    if (msec < 0 || msec > INT_MAX){
        msec = INT_MAX;
    }
    return msec;
}

int Connect::_wait_ready(long msec){
    struct pollfd wfd[1];
    wfd[0].fd = connector->_fd;
    wfd[0].events = POLLOUT;

    if (errno != EINPROGRESS){
        connector->_set_error(CONNECT_ERROR_IO, "_wait_ready(msec)");
        return CONNECT_RT_ERR;
    }

    int res;

    if ((res = poll(wfd, 1, msec)) == -1){
        connector->_set_error(CONNECT_ERROR_IO, "poll(2)");
        connector->closeConnect();
        return CONNECT_RT_ERR;

    }else if (res == 0){
        errno = ETIMEDOUT;
        connector->_set_error(CONNECT_ERROR_IO, "pool(2) timeout");
        connector->closeConnect();
        return CONNECT_RT_ERR;
    }

    if (_check_connect_done(&res) != CONNECT_RT_OK || res == 0){
        _check_socket_error();
        return CONNECT_RT_ERR;
    }
    return CONNECT_RT_OK;
}

int Connect::_check_connect_done(int* completed){
    int rc = ::connect(connector->_fd, reinterpret_cast<const struct sockaddr*>(connector->_addr), connector->_addrlen);
    if (rc == 0){
        *completed = 1;
        return CONNECT_RT_OK;
    }

    switch (errno){
    case EISCONN:
        *completed = 1;
        break;
    case EALREADY:
    case EINPROGRESS:
    case EWOULDBLOCK:
        *completed = 0;
        return CONNECT_RT_OK;
    default:
        return CONNECT_RT_ERR;
    }
    return CONNECT_RT_OK;
}

int Connect::_check_socket_error(){
    int err = 0;
    int errno_saved = errno;
    socklen_t errlen = sizeof(err);

    if (getsockopt(connector->_fd, SOL_SOCKET, SO_ERROR, &err, &errlen) == -1){
        connector->_set_error(CONNECT_ERROR_IO, "getsockopt(SO_ERROR)");
        return CONNECT_RT_ERR;
    }

    if (err == 0){
        err = errno_saved;
    }

    if (err){
        errno = err;
        connector->_set_error(CONNECT_ERROR_IO, "check_socket_error(errno)");
        return CONNECT_RT_ERR;
    }
    return CONNECT_RT_OK;
}

timeval Connect::_to_timeval(const std::chrono::milliseconds& duration) const {
    auto sec = std::chrono::duration_cast<std::chrono::seconds>(duration);
    auto msec = std::chrono::duration_cast<std::chrono::microseconds>(duration - sec);
    timeval t;
    t.tv_sec = sec.count();
    t.tv_usec = msec.count();
    return t;
}

int Connect::_set_blocking(bool blocking){
    int flags;
    if ((flags = fcntl(connector->_fd, F_GETFL)) == -1){
        connector->_set_error(CONNECT_ERROR_IO, "fcntl(F_GETFL)");
        connector->closeConnect();
        return CONNECT_RT_ERR;
    }
    if (blocking){
        flags &= ~O_NONBLOCK; // 阻塞
    }else{
        std::cout << "_set_blocking O_NONBLOCK" << std::endl;
        flags |= O_NONBLOCK; // 非阻塞
    }

    if (fcntl(connector->_fd, F_SETFL, flags) == -1){
        connector->_set_error(CONNECT_ERROR_IO,"fcntl(F_SETFL)");
        connector->closeConnect();
        return CONNECT_RT_ERR;
    }
    return CONNECT_RT_OK;
}

}
}