#include "Point.h"

Point::Point(int x, int y)
{
	this->x = x;
	this->y = y;
}

Point::Point(double x, double y)
{
	this->x = static_cast<int>(x);
	this->y = static_cast<int>(y);
}

Point::Point()
	: Point(0, 0) { }

Point::~Point()
{

}

Point Point::operator+(const Point &p) const
{
	return Point(this->x + p.x, this->y + p.y);
}

Point Point::operator*(double scale) const
{
	auto newX = static_cast<int>(static_cast<double>(this->x) * scale);
	auto newY = static_cast<int>(static_cast<double>(this->y) * scale);
	return Point(newX, newY);
}

int Point::getX() const
{
	return this->x;
}

int Point::getY() const
{
	return this->y;
}

void Point::setX(int x)
{
	this->x = x;
}

void Point::setY(int y)
{
	this->y = y;
}
