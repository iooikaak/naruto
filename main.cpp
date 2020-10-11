#include <iostream>

#include "naruto.h"

int main()
{

    std::cout << "main" << std::endl;
    naruto::Naruto s(7290,521,32);
    s.run();
    return 0;
}
