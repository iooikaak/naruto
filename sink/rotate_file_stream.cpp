//
// Created by kwins on 2020/11/6.
//

#include <dirent.h>
#include <algorithm>

#include "rotate_file_stream.h"
#include "utils/errors.h"

namespace naruto::sink {

RotateFileStream::RotateFileStream(const std::string &dir, long long int rotate_aof_file_size) {
    dir_ = dir;
    if (dir[dir.size()-1] != '/'){
        dir_ += "/";
    }
    rotate_aof_file_size_ = rotate_aof_file_size;
    _rotate_init();
}

long long RotateFileStream::write(const char * s, size_t n) {
    stream_->write(s, n);
    auto size = stream_->tellp();
    if (size >= rotate_aof_file_size_){ _rotate(); }
    return size;
}

void RotateFileStream::flush() { if (stream_){ stream_->flush(); } }

void RotateFileStream::_rotate() {
    if (stream_){
        stream_->flush();
        stream_->close();
    }
    rotate_aof_idx_++;
    cur_aof_file_ = _gen_aof_filename(rotate_aof_idx_);
    stream_ = std::make_shared<std::ofstream>(cur_aof_file_,std::ios::binary|std::ios::app|std::ios::out);
}

std::string RotateFileStream::_gen_aof_filename(int idx) { return dir_ + "naruto.aof." + std::to_string(idx); }

void RotateFileStream::_rotate_init() {
    DIR* dir = opendir(dir_.c_str());
    unsigned char isfile =0x8;
    dirent* ptr;
    std::vector<std::string> list;
    while ((ptr = readdir(dir)) != nullptr){
        std::string name(ptr->d_name);
        if (ptr->d_type == isfile && name.find("aof") != std::string::npos){
            list.emplace_back(ptr->d_name);
        }
    }

    std::sort(list.begin(), list.end(), [](const std::string& x, const std::string& y){
        return x < y;
    });

//    std::for_each(list.begin(), list.end(), [](const std::string& x){
//        std::cout << x << std::endl;
//    });

    if (!list.empty()){
        auto last = list[list.size()-1];
        auto pos = last.find_last_of(".");
        if (pos == std::string::npos){
            throw utils::Error("bad file:" + last);
        }
        auto stridx = last.substr(pos+1);
        rotate_aof_idx_ = std::stoi(stridx) + 1;
    }else{
        rotate_aof_idx_ = 0;
    }

    cur_aof_file_ = _gen_aof_filename(rotate_aof_idx_);
    stream_ = std::make_shared<std::ofstream>(cur_aof_file_,std::ios::binary|std::ios::app|std::ios::out);
    std::cout << "cur_aof_file_:" << cur_aof_file_ << "    rotate_aof_idx_:" << rotate_aof_idx_ << std::endl;
    closedir(dir);
}

}