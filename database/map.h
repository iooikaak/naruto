//
// Created by 王振奎 on 2020/10/6.
//

#ifndef NARUTO_MAP_H
#define NARUTO_MAP_H

#include <unordered_map>

#include "object.h"
#include "utils/errors.h"
#include "protocol/data.pb.h"

namespace naruto {
namespace database {

class Map : public ::naruto::database::object{
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

    ~Map() override;

private:
    std::unordered_map<std::string, data::VALUE> _data;
};

}
}

#endif //NARUTO_MAP_H

