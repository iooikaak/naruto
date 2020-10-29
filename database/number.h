//
// Created by 王振奎 on 2020/10/6.
//

#ifndef NARUTO_NUMBER_H
#define NARUTO_NUMBER_H

#include <atomic>
#include "object.h"
#include "utils/errors.h"

namespace naruto::database {

class Number : public object{
public:
    explicit Number(int64_t v = 0) : data_(v){}

    data::TYPE type() override;
    void serialize(utils::Bytes &bytes) override;
    void deSeralize(utils::Bytes &bytes) override;
    void debugString() override;

    int64_t get() const;
    void set(const int64_t);
    void incr(const int64_t);

    ~Number() override = default;

private:
    std::atomic_int64_t data_;
};

}

#endif //NARUTO_NUMBER_H
