//
// Created by kwins on 2020/11/6.
//

#ifndef NARUTO_RING_BUFFER_H
#define NARUTO_RING_BUFFER_H

#include <vector>
#include <gflags/gflags.h>

DECLARE_int32(repl_aof_ring_size);

namespace naruto::utils{
template <typename T>
class RingBuffer {
public:
    explicit RingBuffer(int capacity = FLAGS_repl_aof_ring_size);
    void put(T);

private:
    bool full();
    int capacity_;
    int put_;
    int get_;
    std::vector<T> buf_;
};

}



#endif //NARUTO_RING_BUFFER_H
