#include <iostream>
#include <vector>

#include "point.h"

Point getPoint()
{
    return Point(3, 4);
}

int main()
{
    std::cout << "Hello World" << __FILE__ << __FUNCTION__ << __LINE__ << std::endl;

    Point *points = new Point[3];
    points[0].m_x = 0;
    points[0].m_y = 100;
    points[1].m_x = 1;
    points[1].m_y = 100;
    points[2].m_x = 2;
    points[2].m_y = 100;

    delete points;
    std::cout << *points[1].p << std::endl;

    std::cout << "hello world.\n"
              << std::endl;
    return 0;
}