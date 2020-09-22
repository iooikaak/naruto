//
// Created by 王振奎 on 2020/9/16.
//
#include <glog/logging.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include "net.h"

int naruto::utils::Net::listen(int port, int tcp_backlog) {
    LOG(INFO) << "net listen port:" << port << " tcp_backlog:" << tcp_backlog;

    struct sockaddr_in addr{};
    int addr_len = sizeof(addr);
    int sd;
    if ((sd = ::socket(PF_INET, SOCK_STREAM, 0)) < 0){
        LOG(ERROR) << "net listen:" << strerror(errno);
        return -1;
    }

    LOG(INFO) << "net listen bind sd:" << sd;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (setSocketReuseAddr(sd) == -1){
        LOG(ERROR) << "_listen _set_socket error:" << strerror(errno);
        return -1;
    }

    if (::bind(sd, (const struct sockaddr*)&addr, addr_len) != 0){
        LOG(ERROR) << "_listen bind error:" << strerror(errno);
        return -1;
    }

    if (::listen(sd, tcp_backlog) < 0){
        LOG(ERROR) << "_listen error:" << strerror(errno);
        return -1;
    }
    return sd;
}

int naruto::utils::Net::setSocketReuseAddr(int fd) {
    int yes = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) return -1;
    int flags;
    if ((flags = fcntl(fd, F_GETFL)) == -1) return -1;
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) return -1;
    return 0;
}
