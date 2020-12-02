//
// Created by kwins on 2020/11/23.
//

#include "file.h"

#include <dirent.h>
#include <string>
#include <glog/logging.h>
#include <fstream>

void naruto::utils::File::listAof(const std::string &dir, std::vector<std::string>& list, const std::string& after) {
    DIR* dirp = opendir(dir.c_str());
    dirent* ptr;
    int after_idx = -1;
    if (!after.empty()){
        after_idx = parseFileName(after);
    }
    while ((ptr = readdir(dirp)) != nullptr){
        std::string name(ptr->d_name);
        if (ptr->d_type == DT_REG && name.find("aof") != std::string::npos){
            int idx;
            if ((idx = parseFileName(ptr->d_name))!= -1){
                if (after_idx != -1){ // 只获取 after和after之后的aof文件
                    if (idx >= after_idx){
                        list.emplace_back(ptr->d_name);
                    }
                }else{
                    list.emplace_back(ptr->d_name);
                }
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

std::string naruto::utils::File::nextAof(const std::string &aofname) {
    int idx;
    if ((idx = parseFileName(aofname)) == -1) return "";
    return "naruto.aof."+ std::to_string(idx + 1);
}

bool naruto::utils::File::hasNextAof(const std::string &dir, std::string &aofname) {
    std::string next = nextAof(aofname);
    if (next.empty()) return false;
    if (size(dir + "/" + next) == -1) return false;
    aofname = next;
    return true;
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
                                   uint64_t offset,
                                   const std::function<void(uint16_t flag, uint16_t type, const unsigned char * s, size_t n)>& f) {
    std::ifstream in(filename, std::ios::binary|std::ios::in);
    if (!in.is_open()) return;
    in.seekg(offset,std::ios::beg);
    in.peek();
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

void naruto::utils::File::saveJson(nlohmann::json &j, const std::string &filename) {
    std::ofstream out(filename, std::ios::in|std::ios::trunc);
    out << j.dump(4);
    out.close();
}

int64_t naruto::utils::File::size(const std::string &filename) {
    std::ifstream in(filename, std::ios::in);
    if (!in.is_open()) return -1;
    in.seekg(0, std::ios::end);
    auto pos = in.tellg();
    in.close();
    return pos;
}
