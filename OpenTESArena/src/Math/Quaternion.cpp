#include <cmath>

#include "Constants.h"
#include "Quaternion.h"

Quaternion::Quaternion(double x, double y, double z, double w)
{
	this->x = x;
	this->y = y;
	this->z = z;
	this->w = w;
}

Quaternion::Quaternion(const Double3 &v, double w)
	: Quaternion(v.x, v.y, v.z, w) { }

Quaternion::Quaternion(const Double4 &v)
	: Quaternion(v.x, v.y, v.z, v.w) { }

Quaternion::Quaternion()
	: Quaternion(0.0, 0.0, 0.0, 0.0) { }

Quaternion::~Quaternion()
{

}

Quaternion Quaternion::identity()
{
	return Quaternion(0.0, 0.0, 0.0, 1.0);
}

Quaternion Quaternion::fromAxisAngle(const Double3 &v, double w)
{
	if (v.lengthSquared() < EPSILON)
	{
		return Quaternion::identity();
	}

	double halfW = w * 0.5;
	Double3 axis = v.normalized() * std::sin(halfW);
	return Quaternion(axis, std::cos(halfW)).normalized();
}

Quaternion Quaternion::fromAxisAngle(const Double4 &v)
{
	return Quaternion::fromAxisAngle(v.x, v.y, v.z, v.w);
}

Quaternion Quaternion::fromAxisAngle(double x, double y, double z, double w)
{
	return Quaternion(Double3(x, y, z), w);
}

Quaternion Quaternion::operator *(const Quaternion &q) const
{
	Double3 left(this->x, this->y, this->z);
	Double3 right(q.x, q.y, q.z);
	Double3 axis = (left * q.w) + (right * this->w) + left.cross(right);
	double magnitude = (this->w * q.w) - left.dot(right);
	return Quaternion(axis, magnitude);
}

std::string Quaternion::toString() const
{
	return std::string("[") +
		std::to_string(this->x) + std::string(", ") +
		std::to_string(this->y) + std::string(", ") +
		std::to_string(this->z) + std::string(", ") +
		std::to_string(this->w) + std::string("]");
}

double Quaternion::length() const
{
	return std::sqrt(
		(this->x * this->x) + 
		(this->y * this->y) + 
		(this->z * this->z) +
		(this->w * this->w));
}

Quaternion Quaternion::normalized() const
{
	double lenRecip = 1.0 / this->length();
	return Quaternion(
		this->x * lenRecip,
		this->y * lenRecip,
		this->z * lenRecip,
		this->w * lenRecip);
}
