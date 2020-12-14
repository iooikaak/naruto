//
// Created by kwins on 2020/11/26.
//

#ifndef NARUTO_BASIC_H
#define NARUTO_BASIC_H

#include <string>
#include <chrono>
#define DEFAULT_RUN_ID_LEN 40

namespace naruto::utils {
using namespace std::chrono;

class Basic {
public:
    static std::string genRandomID(unsigned int len = DEFAULT_RUN_ID_LEN);
    static timeval to_timeval(const std::chrono::milliseconds&);
    static int64_t to_ms(const std::chrono::system_clock::time_point& time);
};

}


#endif //NARUTO_BASIC_H
