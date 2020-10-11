//
// Created by 王振奎 on 2020/8/17.
//

#ifndef NARUTO_UT_CPP_H
#define NARUTO_UT_CPP_H

#include <gtest/gtest.h>
#include <vector>
#include <iostream>


struct data {
    int a;
    std::string *str;
};

class A {
public:
    A() : data_(new data{10, new std::string("1231213")}) {
        std::cout << "A()" << std::endl;
    }

    A(const A &a) = delete;

    A &operator=(const A &a) = delete;

    A(A &&a) noexcept {
        data_ = std::move(a.data_);
        std::cout << "A(A&& a)" << std::endl;
    }

    A &operator=(A &&a) noexcept {
        std::cout << "A& operator= (AA& a)" << std::endl;
        data_ = std::move(a.data_);
        return *this;
    }

    using data_ptr = std::unique_ptr<data>;
    data_ptr data_;
};

A getA() {
    A a;
    return a;
}

class TestCPP : public ::testing::Test {

};

TEST_F(TestCPP, cpp) {
    A a = getA();
    std::cout << "a.a:" << a.data_->a << std::endl;
    std::cout << "a.str:" << *a.data_->str << std::endl;

    A b = std::move(a);
    std::cout << "b.a:" << b.data_->a << std::endl;
    std::cout << "b.str:" << *b.data_->str << std::endl;

//    std::cout <<"a.a:"<< a.data_->a << std::endl;
//    std::cout << "a.str:" << *a.data_->str << std::endl;
}


#endif //NARUTO_UT_CPP_H
