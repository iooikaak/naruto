//
// Created by kwins on 2020/11/6.
//

#ifndef NARUTO_RING_BUFFER_H
#define NARUTO_RING_BUFFER_H

#include <vector>

namespace naruto::utils{
template <typename T>
class RingBuffer {
public:
    explicit RingBuffer(int capacity);
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
