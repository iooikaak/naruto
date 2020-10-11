//
// Created by 王振奎 on 2020/10/3.
//

#ifndef NARUTO_LIST_INT_H
#define NARUTO_LIST_INT_H

#include <vector>

#include "object.h"
#include "protocol/message.pb.h"
#include "utils/errors.h"
#include "utils/bytes.h"

namespace naruto::database{

class ListINT : public ::naruto::database::object {
public:
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

    ~ListINT() override = default;

private:
    std::vector<int64_t> _data;
};


}



#endif //NARUTO_LIST_INT_H
