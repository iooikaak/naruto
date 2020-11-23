//
// Created by kwins on 2020/11/23.
//

#ifndef NARUTO_FILE_H
#define NARUTO_FILE_H

#include <vector>
#include "bytes.h"

namespace naruto::utils {

class File {
public:
    static void listAof(const std::string &dir, std::vector<std::string> &);

    static int parseFileName(const std::string &);

    static void loadFile(const std::string& filename, const std::function<void(uint16_t,uint16_t,const unsigned char*, size_t)>&);
};

}

#endif //NARUTO_FILE_H
