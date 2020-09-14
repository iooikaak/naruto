//
// Created by 王振奎 on 2020/8/31.
//

#ifndef NARUTO_OBJECT_H
#define NARUTO_OBJECT_H

struct object{
    unsigned type:4;
    unsigned encoding:4;
    unsigned lru:24;
    void *ptr;
};

#endif //NARUTO_OBJECT_H
