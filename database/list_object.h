//
// Created by kwins on 2020/10/28.
//

#ifndef NARUTO_LIST_OBJECT_H
#define NARUTO_LIST_OBJECT_H

#include <vector>
#include <shared_mutex>
#include "object.h"
#include "utils/errors.h"

namespace naruto::database {

template <typename T, data::TYPE DT>
class ListObject : public object {
public:
    data::TYPE type() override { return DT; }
    void serialize(utils::Bytes &bytes) override;
    void deSeralize(utils::Bytes &bytes) override;
    void debugString() override;

    int len();
    T lpop();
    void ltrim(int start, int end);
    void lpush(const T&);
    void lrange(int start, int end, std::vector<T>& reply);

private:
    std::shared_mutex mutex_;
    std::vector<T> data_;
};


}



#endif //NARUTO_LIST_OBJECT_H
