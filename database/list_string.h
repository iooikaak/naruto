//
// Created by 王振奎 on 2020/10/6.
//

#ifndef NARUTO_LIST_STRING_H
#define NARUTO_LIST_STRING_H

#include <vector>
#include "object.h"
#include "utils/errors.h"

namespace naruto::database {

class ListString : public object {
public:
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

    ~ListString() override = default;

private:
    std::vector<std::string> _data;
};

}

#endif //NARUTO_LIST_STRING_H
