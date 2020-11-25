//
// Created by kwins on 2020/11/23.
//

#ifndef NARUTO_FILE_H
#define NARUTO_FILE_H

#include <vector>
#include <nlohmann/json.hpp>
#include "bytes.h"

namespace naruto::utils {

class File {
public:
    static void listAof(const std::string &dir, std::vector<std::string> &);

    static int parseFileName(const std::string &);

    static void loadFile(const std::string& filename,  uint64_t offset, const std::function<void(uint16_t,uint16_t,const unsigned char*, size_t)>&);

    static void saveJson(nlohmann::json& j, const std::string& filename);
};

}

#endif //NARUTO_FILE_H
