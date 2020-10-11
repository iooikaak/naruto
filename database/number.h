//
// Created by 王振奎 on 2020/10/6.
//

#ifndef NARUTO_NUMBER_H
#define NARUTO_NUMBER_H

#include "object.h"
#include "utils/errors.h"

namespace naruto::database {

class Number : public object{
public:
    explicit Number(int64_t v = 0) : _data(v){}

    data::TYPE type() override;

    void get(data::VALUE &value) override;

    void set(const data::VALUE &value) override;

    void serialize(utils::Bytes &bytes) override;

    int len() override;

    void lpop(data::VALUE &value) override;

    void ltrim(int start, int end) override;

    void lpush(const data::VALUE &value) override;

    void lrange(int start, int end, data::VALUE &reply) override;

    void incr(int v) override;

    void mapdel(const std::string &string) override;

    void debugString() override ;

    ~Number() override = default;

private:
    int64_t _data;
};

}

#endif //NARUTO_NUMBER_H
