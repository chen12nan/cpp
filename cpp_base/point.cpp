
#include <iostream>
#include "point.h"

Point::Point(int x, int y) : m_x(x), m_y(y)
{
    std::cout << "Point::Point()" << std::endl;
}

Point::~Point()
{
    std::cout << "Point::~Point()----" << m_x << m_y << std::endl;
    delete p;
    p = nullptr;
}

// Point &Point::operator=(Point &&point)
// {
//     std::cout << "operator &&()  " << std::endl;
//     return std::move(point);
// }

Point getPoint()
{
    return Point(3, 4);
}
