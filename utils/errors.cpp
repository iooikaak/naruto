//
// Created by 王振奎 on 2020/8/13.
//
#include <cerrno>

#include "errors.h"

namespace naruto{
namespace utils{

void throw_err(int err_type, const std::string& errmsg){
    switch (err_type)
    {
    case CONNECT_ERROR_IO:
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == ETIMEDOUT){
            throw TimeoutError(errmsg);
        }
        else{
            throw IoError(errmsg);
        }
    case CONNECT_ERROR_OOM:
        throw OomError(errmsg);
    case CONNECT_ERROR_EOF:
        throw ClosedError(errmsg);
    case CONNECT_ERROR_TIMEOUT:
        throw TimeoutError(errmsg);
    case CONNECT_ERROR_PROTOCOL:
        throw ProtoError(errmsg);
    case CONNECT_ERROR_OTHER:
        throw Error(errmsg);
    default:
        throw Error("unknown error code: " + errmsg);
    }
}

}
}