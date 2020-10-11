//
// Created by 王振奎 on 2020/10/6.
//

#ifndef NARUTO_FLOAT_H
#define NARUTO_FLOAT_H

#include "object.h"
#include "utils/errors.h"
#include "utils/bytes.h"

namespace naruto::database {

class Float : public ::naruto::database::object{
public:
    explicit Float(float v = 0.0) : _data(v) {}

    data::TYPE type() override;

    void get(data::VALUE &value) override;

    void set(const data::VALUE &value) override;

    void serialize(::naruto::utils::Bytes &bytes) override;

    int len() override;

    void lpop(data::VALUE &value) override;

    void ltrim(int start, int end) override;

    void lpush(const data::VALUE &value) override;

    void lrange(int start, int end, data::VALUE &reply) override;

    void incr(int v) override;

    void mapdel(const std::string &string) override;

    void debugString() override ;

    ~Float() override = default;

private:
    float _data;
};

}

#endif //NARUTO_FLOAT_H
