//
// Created by kwins on 2020/11/6.
//

#ifndef NARUTO_ROTATE_FILE_STREAM_H
#define NARUTO_ROTATE_FILE_STREAM_H

#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <ios>
#include <fstream>

namespace naruto::sink{

class RotateFileStream {
public:
    explicit RotateFileStream(const std::string& dir, long long rotate_file_size);
    long long write(const char*, size_t);
    void flush();
    static void listAof(const std::string& dir, std::vector<std::string>&);
private:
    void _rotate_init();
    void _rotate();
    std::string _gen_aof_filename(int idx);
    std::shared_ptr<std::ofstream> stream_;
    std::string dir_;
    std::string cur_aof_file_;
    std::string db_file_;
    int rotate_aof_idx_;
    long long rotate_aof_file_size_;
};

}



#endif //NARUTO_ROTATE_FILE_STREAM_H
