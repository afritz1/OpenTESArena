#include "Int3.h"

Int3::Int3(int32_t x, int32_t y, int32_t z)
{
	this->x = x;
	this->y = y;
	this->z = z;
}

Int3::Int3(double x, double y, double z)
{
	this->x = static_cast<int32_t>(x);
	this->y = static_cast<int32_t>(y);
	this->z = static_cast<int32_t>(z);
}

Int3::Int3()
	: Int3(0, 0, 0) { }

Int3::~Int3()
{

}

Int3 Int3::operator +(const Int3 &p) const
{
	return Int3(this->x + p.x, this->y + p.y, this->z + p.z);
}

Int3 Int3::operator -(const Int3 &p) const
{
	return Int3(this->x - p.x, this->y - p.y, this->z - p.z);
}

Int3 Int3::operator *(double scale) const
{
	return Int3(this->x * scale, this->y * scale, this->z * scale);
}

int32_t Int3::getX() const
{
	return this->x;
}

int32_t Int3::getY() const
{
	return this->y;
}

int32_t Int3::getZ() const
{
	return this->z;
}

void Int3::setX(int32_t x)
{
	this->x = x;
}

void Int3::setY(int32_t y)
{
	this->y = y;
}

void Int3::setZ(int32_t z)
{
	this->z = z;
}
