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
#include <sys/socket.h>
#include <cassert>
#include <utility>
#include <glog/logging.h>
#include <arpa/inet.h>

#include "utils/errors.h"
#include "connection.h"

namespace naruto::connection{

Connect::Connect(int fd) {
    _fd = fd;
    LOG(INFO) << "new connect fd:" << _fd;
    _flags = CONNECT_FLAGS_CONNECTED;
    _addr = nullptr;
    _addrlen = 0;
    _err = CONNECT_RT_OK;
    _errmsg = "";
    _opts = ConnectOptions();
    memset(_ibuf,0,CONNECT_READ_BUF_SIZE);
}

Connect::Connect(ConnectOptions opts) : _opts(std::move(opts)) {
    _fd = -1;
    _addr = nullptr;
    _addrlen = 0;
    _err = CONNECT_RT_OK;
    _errmsg = "";
    _flags = CONNECT_FLAGS_INIT;
    memset(_ibuf,0,CONNECT_READ_BUF_SIZE);
}

int Connect::connect() {
    if (_flags & CONNECT_FLAGS_CONNECTED) return CONNECT_RT_OK;

    LOG(INFO) << "Connect::connect---1";

    int client_fd;
    struct sockaddr_in addr{};
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(_opts.host.c_str());
    LOG(INFO) << "connect port:" << _opts.port;
    addr.sin_port = htons(_opts.port);

    int blocking = (_flags & CONNECT_FLAGS_BLOCK);
    int retry_times = 0;

    if ((client_fd = ::socket(PF_INET, SOCK_STREAM, 0)) < 0){
        _set_error(CONNECT_ERROR_OTHER, "socket");
        return CONNECT_RT_ERR;
    }

    LOG(INFO) << "Connect::connect---13";
    retry:
    if (::connect(client_fd, (sockaddr*)&addr, sizeof(addr)) < 0){
        if (errno == EHOSTUNREACH){
            close();
            return CONNECT_RT_ERR;
        }else if (errno == EINPROGRESS){
            if (blocking){
                _wait_ready(_connect_timeout_msec());
            }
        }else if (errno == EADDRNOTAVAIL){
            if (++retry_times >= CONNECT_RETRIES){
                _set_error(CONNECT_ERROR_IO, "retry max times");
                return CONNECT_RT_ERR;
            }else{
                close();
                goto retry;
            }
        }

        _set_error(CONNECT_ERROR_OTHER, "::connect");
        return CONNECT_RT_ERR;
    }

    LOG(INFO) << "Connect::connect---14";
    _fd = client_fd;

    if (_set_blocking(false) != CONNECT_RT_OK){
        _set_error(CONNECT_ERROR_OTHER, "_set_blocking");
        return CONNECT_RT_ERR;
    }

    LOG(INFO) << "Connect::connect---15";
    _flags |= CONNECT_FLAGS_CONNECTED;
    _addr = (struct sockaddr*) malloc(sizeof(struct sockaddr));
    memcpy(_addr, (sockaddr*)&addr, sizeof(struct sockaddr));
    return _set_connect_timeout();
}

void Connect::setOps(const ConnectOptions & ops) { _opts = ops; }

void Connect::reset() noexcept { _err = 0; _errmsg.clear(); }

// 重连
void Connect::reconnect(){
    Connect conn(_opts);

    if (conn.connect() != CONNECT_RT_OK)
        naruto::utils::throw_err(_err, _errmsg);

    assert(!conn.broken());

    conn.reset();
    swap(*this, conn);
}

int Connect::fd() { return _fd; }

auto Connect::last_active() const ->
std::chrono::time_point<std::chrono::steady_clock> { return _last_active; }

void Connect::update_last_active() noexcept { _last_active = std::chrono::steady_clock::now(); }

void swap(Connect &lc, Connect &rc) noexcept {
    std::swap(lc._flags, rc._flags);
    std::swap(lc._fd, rc._fd);
    std::swap(lc._addrlen, rc._addrlen);
    std::swap(lc._addr, rc._addr);
    std::swap(lc._err,rc._err);
    std::swap(lc._errmsg,rc._errmsg);
    std::swap(lc._opts,rc._opts);
    std::swap(lc._last_active, rc._last_active);
}

int Connect::_set_connect_timeout(){
    timeval read_timeout = _to_timeval(_opts.read_timeout);
    timeval write_timeout = _to_timeval(_opts.write_timeout);
    if (setsockopt(_fd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof(read_timeout))){
        _set_error(CONNECT_ERROR_IO,"setsockopt(SO_RCVTIMEO)");
        return CONNECT_RT_ERR;
    }
    if (setsockopt(_fd, SOL_SOCKET, SO_SNDTIMEO, &write_timeout, sizeof(write_timeout))){
        _set_error(CONNECT_ERROR_IO,"setsockopt(SO_SNDTIMEO)");
        return CONNECT_RT_ERR;
    }
    return CONNECT_RT_OK;
}

long Connect::_connect_timeout_msec(){
    long msec = std::chrono::duration_cast<std::chrono::microseconds>(_opts.connect_timeout).count();
    if (msec < 0 || msec > INT_MAX){
        msec = INT_MAX;
    }
    return msec;
}

int Connect::_wait_ready(long msec){
    struct pollfd wfd[1];
    wfd[0].fd = _fd;
    wfd[0].events = POLLOUT;

    if (errno != EINPROGRESS){
        _set_error(CONNECT_ERROR_IO, "_wait_ready(msec)");
        return CONNECT_RT_ERR;
    }

    int res;

    if ((res = poll(wfd, 1, msec)) == -1){
        _set_error(CONNECT_ERROR_IO, "poll(2)");
        close();
        return CONNECT_RT_ERR;

    }else if (res == 0){
        errno = ETIMEDOUT;
        _set_error(CONNECT_ERROR_IO, "pool(2) timeout");
        close();
        return CONNECT_RT_ERR;
    }

    if (_check_connect_done(&res) != CONNECT_RT_OK || res == 0){
        _check_socket_error();
        return CONNECT_RT_ERR;
    }
    return CONNECT_RT_OK;
}

int Connect::_check_connect_done(int* completed){
    int rc = ::connect(_fd, reinterpret_cast<const struct sockaddr*>(_addr), _addrlen);
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

    if (getsockopt(_fd, SOL_SOCKET, SO_ERROR, &err, &errlen) == -1){
        _set_error(CONNECT_ERROR_IO, "getsockopt(SO_ERROR)");
        return CONNECT_RT_ERR;
    }

    if (err == 0){
        err = errno_saved;
    }

    if (err){
        errno = err;
        _set_error(CONNECT_ERROR_IO, "check_socket_error(errno)");
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
    if ((flags = fcntl(_fd, F_GETFL)) == -1){
        _set_error(CONNECT_ERROR_IO, "fcntl(F_GETFL)");
        close();
        return CONNECT_RT_ERR;
    }
    if (blocking){
        flags &= ~O_NONBLOCK; // 阻塞
    }else{
        std::cout << "_set_blocking O_NONBLOCK" << std::endl;
        flags |= O_NONBLOCK; // 非阻塞
    }

    if (fcntl(_fd, F_SETFL, flags) == -1){
        _set_error(CONNECT_ERROR_IO,"fcntl(F_SETFL)");
        close();
        return CONNECT_RT_ERR;
    }
    return CONNECT_RT_OK;
}

void Connect::close() {
    if (_fd != FD_INVALID){
        ::close(_fd);
        _fd = FD_INVALID;
    }
}

// 立即写
void Connect::send(naruto::utils::Bytes & pack) {
    if (_write_pack(pack) != CONNECT_RT_OK)
        naruto::utils::throw_err(_err, _errmsg);
}

void Connect::recv(naruto::utils::Bytes & pack) {
    if (_read_pack(pack) != CONNECT_RT_OK)
        naruto::utils::throw_err(_err, _errmsg);
}

bool Connect::broken() const noexcept { return _err != CONNECT_RT_OK; }

std::string Connect::errmsg() const { return _errmsg; }

int Connect::errcode() const { return _err; }

void Connect::_set_error(int type, const std::string &prefix) {
    _err = type;
    char buf[128] = {0};
    strerror_r(errno, buf, sizeof(buf));
    _errmsg.clear();
    if (!prefix.empty()){
        _errmsg.append(prefix).append(":");
    }
    _errmsg.append(buf);
}

int Connect::_write_pack(naruto::utils::Bytes & pack) {
    LOG(INFO) << "connect write_pack:" << pack.size();
    bool done = false;
    int size = pack.size();
    char* buf = (char*) pack.data();
    int next_len = size;
    ssize_t send_size = 0;
    do{
        if (_err) return CONNECT_RT_ERR;
        ssize_t n = ::send(_fd, &buf[send_size], next_len, 0);
        if (n < 0){
            // 阻塞或者中断
            if ((errno == EWOULDBLOCK && !(_flags & CONNECT_FLAGS_BLOCK)) || errno == EINTR){
                /* Try again later */
            }else{
                _set_error(CONNECT_ERROR_IO, "send(buf)");
                return CONNECT_RT_ERR;
            }
        } else if ( n > 0 ){
            send_size += n;
            if (send_size == size){
                done = true;
            }else{
                next_len = size - send_size;
            }
        }
    }while(!done);

    return CONNECT_RT_OK;
}

int Connect::_read_pack(naruto::utils::Bytes & pack) {
    LOG(INFO) << "connect read_pack:" << pack.size();
    pack.clear();
    bool done = false;
    ssize_t readed_num = 0;
    size_t next_len = PACK_HEAD_LEN;
    do{
        readed_num = ::recv(_fd, _ibuf, next_len, 0);
        if (readed_num == -1){
            if ((errno == EWOULDBLOCK && !(_flags & CONNECT_FLAGS_BLOCK)) || errno == EINTR){
                /* Try again later */
            }else if (errno == ETIMEDOUT && (_flags & CONNECT_FLAGS_BLOCK)){
                _set_error(CONNECT_ERROR_TIMEOUT, "_read_pack timeout");
                return CONNECT_RT_ERR;
            }else{
                _set_error(CONNECT_ERROR_IO,"_read_pack");
                return CONNECT_RT_ERR;
            }
        }else if (readed_num == 0){
            _set_error(CONNECT_ERROR_EOF, "server closed the connection");
            return CONNECT_RT_ERR;

        }else if (readed_num > 0){ // 只读一个完整的包，不多读
            pack.putBytes((uint8_t*)_ibuf, readed_num);
            if (pack.size()  >= PACK_HEAD_LEN){
                uint32_t length = pack.getInt(0);
                if (pack.size() == length){ // 拿到了完成的包
                    done = true;
                }
                next_len = length - pack.size();
                if (next_len > CONNECT_READ_BUF_SIZE)
                    next_len = CONNECT_READ_BUF_SIZE;
            }else{
                next_len = PACK_HEAD_LEN - readed_num;
            }
        }
    }while (!done);
    return CONNECT_RT_OK;
}
}