//
// Created by 王振奎 on 2020/9/18.
//

#ifndef NARUTO_RING_BUFFER_H
#define NARUTO_RING_BUFFER_H

#include <vector>


namespace naruto{
namespace utils{

template <typename T>
class RingBuffer {
public:
    RingBuffer(int size);

private:
    std::vector<T> _buf;
    std::size_t
};


}
}


#endif //NARUTO_RING_BUFFER_H
