//
// Created by kwins on 2020/11/6.
//

#include "rotate_file_stream.h"

#include <glog/logging.h>
#include <dirent.h>
#include <algorithm>
#include "utils/errors.h"
#include "utils/file.h"

namespace naruto::sink {

RotateFileStream::RotateFileStream(const std::string &dir, long long int rotate_aof_file_size) {
    dir_ = dir;
    LOG(INFO) << "rotate file stream dir:" << dir_;
    if (dir[dir.size()-1] != '/'){
        dir_ += "/";
    }
    rotate_aof_file_size_ = rotate_aof_file_size;
    _rotate_init();
}

RotateFileStream::fileState RotateFileStream::curRollFile() {
    return fileState{
        .name = cur_aof_file_,
        .offset = stream_->tellp(),
    };
}

long long RotateFileStream::write(const char * s, size_t n) {
    auto size = stream_->tellp();
    if (size >= rotate_aof_file_size_){ _rotate(); }
    stream_->write(s, n);
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
    std::vector<std::string> list;
    utils::File::listAof(dir_, list);

    if (!list.empty()){
        auto last = list[list.size()-1];
        auto stridx = utils::File::parseFileName(last);
        rotate_aof_idx_ = stridx + 1;
    }else{
        rotate_aof_idx_ = 0;
    }

    cur_aof_file_ = _gen_aof_filename(rotate_aof_idx_);
    stream_ = std::make_shared<std::ofstream>(cur_aof_file_,std::ios::binary|std::ios::app|std::ios::out);
}

}