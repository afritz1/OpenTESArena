#ifndef POINT_H
#define POINT_H

class Point
{
private:
	int x, y;
public:
	Point(int x, int y);
	Point(double x, double y);
	Point();
	~Point();

	Point operator +(const Point &p) const;
	Point operator *(double scale) const;

	int getX() const;
	int getY() const;

	void setX(int x);
	void setY(int y);
};

#endif