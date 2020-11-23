//
// Created by kwins on 2020/11/23.
//

#include "file.h"

#include <dirent.h>
#include <string>
#include <glog/logging.h>
#include <fstream>

void naruto::utils::File::listAof(const std::string &dir, std::vector<std::string>& list) {
    DIR* dirp = opendir(dir.c_str());
    dirent* ptr;
    while ((ptr = readdir(dirp)) != nullptr){
        std::string name(ptr->d_name);
        if (ptr->d_type == DT_REG && name.find("aof") != std::string::npos){
            if (parseFileName(ptr->d_name) != -1){
                list.emplace_back(ptr->d_name);
            }
        }
    }

    std::sort(list.begin(), list.end(), [](const std::string& x, const std::string& y){
        auto xidx = parseFileName(x);
        auto yidx = parseFileName(y);
        return xidx < yidx;
    });
    closedir(dirp);
}

int naruto::utils::File::parseFileName(const std::string & v) {
    auto pos = v.find_last_of('.');
    if (pos == std::string::npos){
        LOG(WARNING) << "Bad Aof file name:" << v;
        return -1;
    }
    auto stridx = v.substr(pos+1);
    return std::stoi(stridx);
}

void naruto::utils::File::loadFile(const std::string &filename,
                                   const std::function<void(uint16_t flag, uint16_t type, const unsigned char * s, size_t n)>& f) {
    std::ifstream in(filename, std::ios::binary|std::ios::in);
    if (!in.is_open()) return;

    char buf[4096];
    utils::Bytes bytes;
    while (!in.eof()){
        in.read(buf, sizeof(buf));
        bytes.putBytes((uint8_t*)buf, in.gcount());
        while (bytes.bytesRemaining() > PACK_SIZE_LEN){
            uint32_t pack_size = bytes.getInt();
            if (bytes.bytesRemaining() < pack_size - sizeof(pack_size)){
                break; // 读取不够一个包
            }
            uint16_t flag = bytes.getShort();
            uint16_t type = bytes.getShort();

            unsigned body_size = pack_size - PACK_HEAD_LEN;
            unsigned char msg[body_size];
            bytes.getBytes((uint8_t*)msg, body_size);
            f(flag, type, msg, body_size);
        }
    }
    in.close();
}
