#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <iostream>

void clear_line()
{
    std::cout << "\r";
    std::cout << std::string(80, ' ');
    std::cout << "\r";
}

#endif //UTILS_H
