//
// Created by 王振奎 on 2020/8/10.
//

#ifndef NARUTO_UNITTEST_UT_EV_CLIENT_H
#define NARUTO_UNITTEST_UT_EV_CLIENT_H
#include <gtest/gtest.h>
#include <glog/logging.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>
#include <netinet/in.h>
#include <glog/logging.h>
#include <string>
#include <thread>
#include <chrono>
#include <cstdio>

#define  CLIENT_PORT_NO 7290
#define CLIENT_BUFFER_SIZE 1024
using namespace std;

class TestLibevClient : public ::testing::Test{
public:

};

TEST_F(TestLibevClient, client_ev){
    int client_fd;
    int len;
    sockaddr_in addr;
    char buf[CLIENT_BUFFER_SIZE];
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    LOG(INFO) << "connect port:" << CLIENT_PORT_NO;
    addr.sin_port = htons(CLIENT_PORT_NO);

    if ((client_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        LOG(ERROR) << "client socket error";
        return;
    }

    if (connect(client_fd, (sockaddr*)&addr, sizeof(addr)) < 0){
        LOG(ERROR) << "client connect error";
        return;
    }

    string line;
    while (true){
        line.clear();
        LOG(INFO) << "Enter string to send:";
        getline(cin, line);

        LOG(INFO) << "Enter:" << line;
        if (line == "quit") break;

        len = send(client_fd, line.c_str(), line.size(), 0);
        LOG(INFO) << "Send " << client_fd << " success, len:" << line.size();
        this_thread::sleep_for(chrono::seconds(2));
    }

    LOG(INFO) << "Client quit";
    close(client_fd);
}
#endif //NARUTO_UNITTEST_UT_EV_CLIENT_H
