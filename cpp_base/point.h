
#ifndef __POINT_H__
#define __POINT_H__

class Point
{
public:
    Point(int x, int y);
    Point() { p = new int(10); }
    ~Point();
    // Point &operator=(Point &&point);

    int m_x = 0;
    int m_y = 0;
    int *p = nullptr;
};

Point getPoint();

#endif // __POINT_H__

