#ifndef __POINT_H__
#define __POINT_H__

#include <iostream>

class Point
{
public:
    Point(int x, int y) { std::cout << "Constructor" << std::endl; }
    ~Point() { std::cout << "Destructor-----------------" << std::endl; }
    Point(const Point &point) { std::cout << " Copy constructor  " << std::endl; }
};

#endif // __POINT_H__
