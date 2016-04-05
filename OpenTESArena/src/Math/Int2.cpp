#include "Int2.h"

Int2::Int2(int x, int y)
{
	this->x = x;
	this->y = y;
}

Int2::Int2(double x, double y)
{
	this->x = static_cast<int>(x);
	this->y = static_cast<int>(y);
}

Int2::Int2()
	: Int2(0, 0) { }

Int2::~Int2()
{

}

Int2 Int2::operator+(const Int2 &p) const
{
	return Int2(this->x + p.x, this->y + p.y);
}

Int2 Int2::operator-(const Int2 &p) const
{
	return Int2(this->x - p.x, this->y - p.y);
}

Int2 Int2::operator*(double scale) const
{
	auto newX = static_cast<int>(static_cast<double>(this->x) * scale);
	auto newY = static_cast<int>(static_cast<double>(this->y) * scale);
	return Int2(newX, newY);
}

int Int2::getX() const
{
	return this->x;
}

int Int2::getY() const
{
	return this->y;
}

void Int2::setX(int x)
{
	this->x = x;
}

void Int2::setY(int y)
{
	this->y = y;
}
