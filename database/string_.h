//
// Created by 王振奎 on 2020/10/6.
//

#ifndef NARUTO_STRING__H
#define NARUTO_STRING__H

#include "object.h"
#include "utils/errors.h"

namespace naruto {
namespace database {

class String : public ::naruto::database::object {
public:
    explicit String(std::string v = "") : _data(std::move(v)) {}
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

private:
    std::string _data;
};

}
}



#endif //NARUTO_STRING__H
