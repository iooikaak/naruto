//
// Created by kwins on 2020/11/26.
//

#include "basic.h"

#include <fcntl.h>
#include <unistd.h>

std::string naruto::utils::Basic::genRandomID(unsigned int len) {
    FILE *fp = fopen("/dev/urandom","r");
    std::string charset = "0123456789abcdef";
    auto* p = (unsigned char*)malloc(len + 1);
    unsigned int j;
    if (fp == nullptr || fread(p, len, 1, fp) == 0){
        auto now = system_clock::now();
        auto millisecs = duration_cast<milliseconds>(now.time_since_epoch());

        auto* x = p;
        unsigned int l = len;

        timeval tv{};
        pid_t pid = getpid();

        tv.tv_sec = millisecs.count() / 1000;
        tv.tv_usec = (millisecs.count() % 1000) * 1000;

        if (l >= sizeof(pid)) {
            memcpy(x,&pid,sizeof(pid));
            l -= sizeof(pid);
            x += sizeof(pid);
        }

        if (l >= sizeof(tv.tv_usec)) {
            memcpy(x,&tv.tv_usec,sizeof(tv.tv_usec));
            l -= sizeof(tv.tv_usec);
            x += sizeof(tv.tv_usec);
        }

        if (l >= sizeof(tv.tv_sec)) {
            memcpy(x,&tv.tv_sec,sizeof(tv.tv_sec));
            l -= sizeof(tv.tv_sec);
            x += sizeof(tv.tv_sec);
        }

        for (j = 0; j < len; ++j) {
            p[j] ^= (unsigned int )rand();
        }
    }
    for (j = 0; j < len; ++j) {
        p[j] = charset[p[j] & 0x0F];
    }
    p[j+1] = '\0';
    if (fp) fclose(fp);
    std::string str((const char*)p);
    return str;
}

timeval naruto::utils::Basic::to_timeval(const std::chrono::milliseconds & duration) {
    timeval tv{};
    tv.tv_sec = duration.count() / 1000;
    tv.tv_usec = (duration.count() % 1000) * 1000;
    return tv;
}

int64_t naruto::utils::Basic::to_ms(const std::chrono::system_clock::time_point &time) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(time.time_since_epoch()).count();
}
