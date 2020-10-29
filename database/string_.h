//
// Created by 王振奎 on 2020/10/6.
//

#ifndef NARUTO_STRING__H
#define NARUTO_STRING__H

#include "object.h"
#include "utils/errors.h"

namespace naruto::database {

class String : public ::naruto::database::object {
public:
    explicit String(std::string v = "") : _data(std::move(v)) {}

    data::TYPE type() override;

    void serialize(::naruto::utils::Bytes &bytes) override;

    void deSeralize(utils::Bytes &bytes) override;

    void debugString() override ;

    std::string get();

    void set(const std::string&);
private:
    std::string _data;
};

}



#endif //NARUTO_STRING__H
