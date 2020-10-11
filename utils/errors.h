//
// Created by 王振奎 on 2020/8/13.
//

#ifndef NARUTO__ERRORS_H
#define NARUTO__ERRORS_H

#include <exception>
#include <string>

// 定义 CONNECT 错误值
#define CONNECT_ERROR_IO 1
#define CONNECT_ERROR_OTHER 2
#define CONNECT_ERROR_EOF 3
#define CONNECT_ERROR_PROTOCOL 4
#define CONNECT_ERROR_OOM 5
#define CONNECT_ERROR_TIMEOUT 6
#define CONNECT_ERROR_EAGIN_TIMEOUT 7

#define ERROR_NOT_SUPPORT_TYPE 8

namespace naruto::utils{

class Error : public std::exception{
public:
    explicit Error(std::string msg) : _msg(std::move(msg)) {}

    Error(const Error &) = default;
    Error& operator=(const Error &) = default;

    Error(Error &&) = default;
    Error& operator=(Error &&) = default;

    virtual ~Error() = default;

    virtual const char* what() const noexcept {
        return _msg.data();
    }

private:
    std::string _msg;
};

class IoError : public Error {
public:
    explicit IoError(const std::string &msg) : Error(msg) {}

    IoError(const IoError &) = default;
    IoError& operator=(const IoError &) = default;

    IoError(IoError &&) = default;
    IoError& operator=(IoError &&) = default;

    virtual ~IoError() = default;
};

class TimeoutError : public IoError {
public:
    explicit TimeoutError(const std::string &msg) : IoError(msg) {}

    TimeoutError(const TimeoutError &) = default;
    TimeoutError& operator=(const TimeoutError &) = default;

    TimeoutError(TimeoutError &&) = default;
    TimeoutError& operator=(TimeoutError &&) = default;

    virtual ~TimeoutError() = default;
};

class ClosedError : public Error {
public:
    explicit ClosedError(const std::string &msg) : Error(msg) {}

    ClosedError(const ClosedError &) = default;
    ClosedError& operator=(const ClosedError &) = default;

    ClosedError(ClosedError &&) = default;
    ClosedError& operator=(ClosedError &&) = default;

    virtual ~ClosedError() = default;
};


class ProtoError : public Error {
public:
    explicit ProtoError(const std::string &msg) : Error(msg) {}

    ProtoError(const ProtoError &) = default;
    ProtoError& operator=(const ProtoError &) = default;

    ProtoError(ProtoError &&) = default;
    ProtoError& operator=(ProtoError &&) = default;

    virtual ~ProtoError() = default;
};

class OomError : public Error {
public:
    explicit OomError(const std::string &msg) : Error(msg) {}

    OomError(const OomError &) = default;
    OomError& operator=(const OomError &) = default;

    OomError(OomError &&) = default;
    OomError& operator=(OomError &&) = default;

    virtual ~OomError() = default;
};

class ReplyError : public Error {
public:
    explicit ReplyError(const std::string &msg) : Error(msg) {}

    ReplyError(const ReplyError &) = default;
    ReplyError& operator=(const ReplyError &) = default;

    ReplyError(ReplyError &&) = default;
    ReplyError& operator=(ReplyError &&) = default;

    virtual ~ReplyError() = default;
};

class TypeError : public Error{
public:
    explicit TypeError(const std::string& msg) : Error(msg){}
    TypeError(const TypeError &) = default;
    TypeError& operator=(const TypeError &) = default;

    TypeError(TypeError &&) = default;
    TypeError& operator=(TypeError &&) = default;

    virtual ~TypeError() = default;
};


class BadArgError : public Error{
public:
    explicit BadArgError(const std::string& msg) : Error(msg){}
    BadArgError(const BadArgError &) = default;
    BadArgError& operator=(const BadArgError &) = default;

    BadArgError(BadArgError &&) = default;
    BadArgError& operator=(BadArgError &&) = default;

    virtual ~BadArgError() = default;
};

void throw_err(int err_type, const std::string& errmsg);

}

#endif //NARUTO__ERRORS_H
