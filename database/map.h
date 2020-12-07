//
// Created by 王振奎 on 2020/10/6.
//

#ifndef NARUTO_MAP_H
#define NARUTO_MAP_H

#include <unordered_map>
#include <shared_mutex>
#include "object.h"
#include "utils/errors.h"

namespace naruto::database {

class Map : public ::naruto::database::object{
public:
    explicit Map(const std::unordered_map<std::string, std::string>& data = {});

    tensorflow::Type type() override;

    std::string typeName() override;

    void serialize(tensorflow::Feature&) override;

    void deSeralize(const tensorflow::Feature&) override;

    void debugString() override ;

    void del(const std::string& field);
    std::string get(const std::string& field);
    void put(const std::string& field, const std::string& v);

    ~Map() override = default;

private:
    std::shared_mutex mutex_;
    std::unordered_map<std::string, std::string> data_;
};

}

#endif //NARUTO_MAP_H

