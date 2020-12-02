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
    static void listAof(const std::string&dir, std::vector<std::string>&, const std::string& after = "");
    static std::string nextAof(const std::string& aofname);
    // 如果 aofname 的下一个文件存在，则 aofname = next aof
    static bool hasNextAof(const std::string& dir, std::string& aofname);
    static int parseFileName(const std::string &);

    static void loadFile(const std::string& filename,  uint64_t offset, const std::function<void(uint16_t,uint16_t,const unsigned char*, size_t)>&);

    static void saveJson(nlohmann::json& j, const std::string& filename);

    static int64_t size(const std::string& file);
};

}

#endif //NARUTO_FILE_H
