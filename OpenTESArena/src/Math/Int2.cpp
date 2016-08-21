#include "Int2.h"

Int2::Int2(int32_t x, int32_t y)
{
	this->x = x;
	this->y = y;
}

Int2::Int2(double x, double y)
{
	this->x = static_cast<int32_t>(x);
	this->y = static_cast<int32_t>(y);
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
	auto newX = static_cast<int32_t>(static_cast<double>(this->x) * scale);
	auto newY = static_cast<int32_t>(static_cast<double>(this->y) * scale);
	return Int2(newX, newY);
}

int32_t Int2::getX() const
{
	return this->x;
}

int32_t Int2::getY() const
{
	return this->y;
}

void Int2::setX(int32_t x)
{
	this->x = x;
}

void Int2::setY(int32_t y)
{
	this->y = y;
}
