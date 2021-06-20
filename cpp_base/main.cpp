
#include <iostream>
#include <vector>

#include "point.h"

Point getPoint()
{
    return Point(3, 4);
}

int main(int argc, char *argv[])
{
    std::cout << "Hello CMake" << std::endl;

    Point point(3, 4);
    // points.emplace_back({3, 4});

    return 0;
}