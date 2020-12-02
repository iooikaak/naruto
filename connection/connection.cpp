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
#include "utils/basic.h"
#include "connection.h"

namespace naruto::connection{

Connect::Connect(int fd) {
    fd_ = fd;
    LOG(INFO) << "new connect fd:" << fd_;
    flag_ |= (unsigned)flags::BLOCK;
    status_ = status::INIT;
    addr_ = nullptr;
    addrlen_ = 0;
    err_ = CONNECT_RT_OK;
    errmsg_ = "";
    opts_ = ConnectOptions();
    memset(_ibuf,0,CONNECT_READ_BUF_SIZE);
}

Connect::Connect(ConnectOptions opts) : opts_(std::move(opts)) {
    fd_ = -1;
    addr_ = nullptr;
    addrlen_ = 0;
    err_ = CONNECT_RT_OK;
    errmsg_ = "";
    flag_ |= (unsigned)flags::BLOCK;
    memset(_ibuf,0,CONNECT_READ_BUF_SIZE);
}

Connect::~Connect() { if (addr_ != nullptr) free(addr_); }

int Connect::connect() {
    if (status_ == status::CONNECTED) return CONNECT_RT_OK;

    int client_fd;
    struct sockaddr_in addr{};
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(opts_.host.c_str());
    addr.sin_port = htons(opts_.port);

    unsigned blocking = (flag_ & (unsigned)flags::BLOCK);
    int retry_times = 0;

    if ((client_fd = ::socket(PF_INET, SOCK_STREAM, 0)) < 0){
        _set_error(CONNECT_ERROR_OTHER, "socket");
        return CONNECT_RT_ERR;
    }

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

    fd_ = client_fd;

    if (_set_blocking(true) != CONNECT_RT_OK){
        _set_error(CONNECT_ERROR_OTHER, "_set_blocking");
        return CONNECT_RT_ERR;
    }

    status_ = status::CONNECTED;
    addr_ = (struct sockaddr*) malloc(sizeof(struct sockaddr));
    memcpy(addr_, (sockaddr*)&addr, sizeof(struct sockaddr));

    LOG(INFO) << "connected port " << opts_.port;
    return _set_connect_timeout();
}

void Connect::setOps(const ConnectOptions & ops) { opts_ = ops; }

std::string Connect::remoteAddr() const {
    sockaddr_in sa{};
    socklen_t len = sizeof(sa);
    ::getpeername(fd(), (struct sockaddr*)&sa, &len);
    char* ip = inet_ntoa(sa.sin_addr);
    int port = ntohs(sa.sin_port);
    return std::string(ip) + ":" + std::to_string(port);
}

std::string Connect::addr() const { return opts_.host + ":" + std::to_string(opts_.port); }

void Connect::reset() noexcept { err_ = 0; errmsg_.clear(); }

// 重连
void Connect::reconnect(){
    Connect conn(opts_);

    if (conn.connect() != CONNECT_RT_OK)
        naruto::utils::throw_err(err_, errmsg_);

    assert(!conn.broken());

    conn.reset();
    swap(*this, conn);
}

int Connect::fd() const { return fd_; }

auto Connect::last_active() const -> std::chrono::steady_clock::time_point { return _last_active; }

void Connect::update_last_active() noexcept { _last_active = std::chrono::steady_clock::now(); }

void swap(Connect &lc, Connect &rc) noexcept {
    std::swap(lc.flag_, rc.flag_);
    std::swap(lc.fd_, rc.fd_);
    std::swap(lc.addrlen_, rc.addrlen_);
    memcpy(lc.addr_, rc.addr_, sizeof(struct sockaddr));
    std::swap(lc.err_, rc.err_);
    std::swap(lc.errmsg_, rc.errmsg_);
    std::swap(lc.status_, rc.status_);
    std::swap(lc.opts_, rc.opts_);
    std::swap(lc._last_active, rc._last_active);
}

int Connect::_set_connect_timeout(){
    timeval read_timeout = utils::Basic::to_timeval(opts_.read_timeout);
    timeval write_timeout = utils::Basic::to_timeval(opts_.write_timeout);
    if (setsockopt(fd_, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof(read_timeout))){
        _set_error(CONNECT_ERROR_IO,"setsockopt(SO_RCVTIMEO)");
        return CONNECT_RT_ERR;
    }
    if (setsockopt(fd_, SOL_SOCKET, SO_SNDTIMEO, &write_timeout, sizeof(write_timeout))){
        _set_error(CONNECT_ERROR_IO,"setsockopt(SO_SNDTIMEO)");
        return CONNECT_RT_ERR;
    }
    return CONNECT_RT_OK;
}

long Connect::_connect_timeout_msec(){
    long msec = std::chrono::duration_cast<std::chrono::microseconds>(opts_.connect_timeout).count();
    if (msec < 0 || msec > INT_MAX){
        msec = INT_MAX;
    }
    return msec;
}

int Connect::_wait_ready(long msec){
    struct pollfd wfd[1];
    wfd[0].fd = fd_;
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
    int rc = ::connect(fd_, const_cast<const struct sockaddr*>(addr_), addrlen_);
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

    if (getsockopt(fd_, SOL_SOCKET, SO_ERROR, &err, &errlen) == -1){
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

int Connect::_set_blocking(bool blocking){
    int flags;
    if ((flags = fcntl(fd_, F_GETFL)) == -1){
        _set_error(CONNECT_ERROR_IO, "fcntl(F_GETFL)");
        close();
        return CONNECT_RT_ERR;
    }
    if (blocking){
        LOG(INFO) << "阻塞";
        flags &= ~O_NONBLOCK; // 阻塞
    }else{
        LOG(INFO) << "非阻塞";
        flags |= O_NONBLOCK; // 非阻塞
    }

    if (fcntl(fd_, F_SETFL, flags) == -1){
        _set_error(CONNECT_ERROR_IO,"fcntl(F_SETFL)");
        close();
        return CONNECT_RT_ERR;
    }
    return CONNECT_RT_OK;
}

void Connect::close() {
    if (fd_ != FD_INVALID){
        ::close(fd_);
        fd_ = FD_INVALID;
    }
    status_ = status::CLOSE;
}

// 立即写
void Connect::send(naruto::utils::Bytes & pack) {
    if (write_(pack) != CONNECT_RT_OK)
        naruto::utils::throw_err(err_, errmsg_);
}

void Connect::recv(naruto::utils::Bytes & pack) {
    if (read_(pack) != CONNECT_RT_OK)
        naruto::utils::throw_err(err_, errmsg_);
}

bool Connect::broken() const noexcept {
    return (err_ != CONNECT_RT_OK) || (status_ != status::CONNECTED);
}

std::string Connect::errmsg() const { return errmsg_; }

int Connect::errcode() const { return err_; }

void Connect::_set_error(int type, const std::string &prefix) {
    err_ = type;
    char buf[128] = {0};
    strerror_r(errno, buf, sizeof(buf));
    errmsg_.clear();
    if (!prefix.empty()){
        errmsg_.append(prefix).append(":");
    }
    errmsg_.append(buf);
}

int Connect::send(const char * buf, size_t n) {
    long size = n;
    long next_len = size;
    ssize_t send_size = 0;
    while (true){
        if (err_) return CONNECT_RT_ERR;
        ssize_t writed = ::send(fd_, &buf[send_size], next_len, MSG_WAITALL);
        if (writed == 0) {
            _set_error(CONNECT_RT_CLOSE, "connect closed");
            return CONNECT_RT_CLOSE;
        } else if (writed < 0){
            // 阻塞或者中断
            if ((errno == EWOULDBLOCK &&
                    !(flag_ & (unsigned)flags::BLOCK)) || errno == EINTR){
                /* Try again later */
            }else{
                _set_error(CONNECT_ERROR_IO, "send(buf)");
                return CONNECT_RT_ERR;
            }
        }else{
            send_size += writed;
            if (send_size == size){
                break;
            }else{
                next_len = size - send_size;
            }
        }
    }
    return CONNECT_RT_OK;
}

int Connect::write_(naruto::utils::Bytes & pack) {
    return send((const char*)pack.data(), pack.size());
}

int Connect::read_(naruto::utils::Bytes& pack) {
    pack.clear();
    ssize_t readn = 0;
    size_t next_len = PACK_HEAD_LEN;
    while (true){
        if (err_) return CONNECT_RT_ERR;
        readn = ::recv(fd_, _ibuf, next_len, MSG_WAITALL);
        if (readn == 0){
            _set_error(CONNECT_RT_CLOSE, "connect closed");
            return CONNECT_RT_CLOSE;
        } else if (readn < 0){
            if ((errno == EWOULDBLOCK && !(flag_ & (unsigned)flags::BLOCK)) || errno == EINTR){
                /* Try again later */
            }else if (errno == ETIMEDOUT && (flag_ & (unsigned)flags::BLOCK)){
                _set_error(CONNECT_ERROR_TIMEOUT, "read_(timeout)");
                return CONNECT_RT_ERR;
            }else{
                _set_error(CONNECT_ERROR_IO,"read_(io)");
                return CONNECT_RT_ERR;
            }
        }else { // 只读一个完整的包，不多读
            pack.putBytes((uint8_t*)_ibuf, readn);
            if (pack.size()  >= PACK_HEAD_LEN){
                uint32_t length = pack.getInt(0);
                if (pack.size() == length) break; // 拿到了完成的包
                next_len = length - pack.size();
                if (next_len > CONNECT_READ_BUF_SIZE) next_len = CONNECT_READ_BUF_SIZE;
            }else{
                next_len = PACK_HEAD_LEN - readn;
            }
        }
    }
    return CONNECT_RT_OK;
}

}