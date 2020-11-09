//
// Created by kwins on 2020/11/6.
//

#include "ring_buffer.h"

namespace naruto::utils{

template<typename T>
RingBuffer<T>::RingBuffer(int capacity) {
    capacity_ = capacity;
    put_ = 0;
    get_ = 0;
    buf_.reserve(capacity_);
}

template<typename T>
void RingBuffer<T>::put(T t) {
    if (full()){

    }
}

template<typename T>
bool RingBuffer<T>::full() {
    return get_ == (put_ + 1) % capacity_;
}

}
