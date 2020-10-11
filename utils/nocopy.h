//
// Created by 王振奎 on 2020/8/24.
//

#ifndef NARUTO_NOCOPY_H
#define NARUTO_NOCOPY_H

namespace naruto::utils {

class UnCopyable {
public:
    UnCopyable() = default;

    UnCopyable(const UnCopyable &) = delete;

    UnCopyable &operator=(const UnCopyable &) = delete;
};
}
#endif //NARUTO_NOCOPY_H
