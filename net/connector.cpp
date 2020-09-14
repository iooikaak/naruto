//
// Created by 王振奎 on 2020/8/14.
//

#include <cstring>
#include <string>
#include <cerrno>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <glog/logging.h>
#include <chrono>
#include <utility>

#include "connector.h"
#include "utils/errors.h"
#include "protocol/message_type.h"

namespace naruto{
namespace net{

Connector::Connector(ConnectOptions  ops):
                        _options(std::move(ops)), _flags(0),
                        _err(0), _errmsg(""),
                        _fd(FD_INVALID), _addr(nullptr),
                        _addrlen(0),_ibuf(){}


Connector::Connector(ConnectOptions&& ops):
                        _options(std::move(ops)), _flags(0),
                        _err(0), _errmsg(""),
                        _fd(FD_INVALID), _addr(nullptr),
                        _addrlen(0),_ibuf(){}

Connector::Connector(ConnectOptions  ops, int fd):
                        _options(std::move(ops)), _flags(0),
                        _err(0), _errmsg(""),
                        _fd(fd), _addr(nullptr),
                        _addrlen(0),_ibuf(){}

Connector::Connector(ConnectOptions&& ops, int fd):
                        _options(std::move(ops)), _flags(0),
                        _err(0), _errmsg(""),
                        _fd(fd), _addr(nullptr),
                        _addrlen(0),_ibuf(){}

Connector::~Connector(){
    LOG(INFO) << "Connector::~Connector";
    if (_addr != nullptr) free(_addr);
}

void Connector::reset() noexcept{ _err = 0; _errmsg.clear(); }
bool Connector::broken() const noexcept { return _err != CONNECT_RT_OK; }

void Connector::send(naruto::utils::Bytes& pack) {
    // 立即写
    if (_write_pack(pack) != CONNECT_RT_OK)
        naruto::utils::throw_err(_err, _errmsg);
}

void Connector::recv(naruto::utils::Bytes& pack) {
    if (_read_pack(pack) != CONNECT_RT_OK)
        naruto::utils::throw_err(_err, _errmsg);
}

int Connector::_read_pack(naruto::utils::Bytes& pack){
    pack.clear();
    bool done = false;
    ssize_t readed_num = 0;
    size_t next_len = PACK_HEAD_LEN;
    std::chrono::milliseconds delay(5);
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
            }
        }
    }while (!done);

    return CONNECT_RT_OK;
}

// 写一个完整的包
int Connector::_write_pack(naruto::utils::Bytes& pack){
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

void Connector::closeConnect() {
    if (_fd != FD_INVALID){
        ::close(_fd);
        _fd = FD_INVALID;
    }
}

void Connector::_set_error(int type, const std::string& prefix){
    _err = type;
    char buf[128] = {0};
    strerror_r(errno, buf, sizeof(buf));
    _errmsg.clear();
    if (!prefix.empty()){
        _errmsg.append(prefix).append(":");
    }
    _errmsg.append(buf);
}

std::string Connector::_remote_addr() {
    struct sockaddr_in* in = (struct sockaddr_in*)_addr;
    char str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &in->sin_addr, str, sizeof(str));
    return std::string(str) + ":" + std::to_string(in->sin_port);
}

}
}
