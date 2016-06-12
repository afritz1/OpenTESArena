#include <cmath>

#include "Quaternion.h"

#include "Constants.h"
#include "Float3.h"
#include "Float4.h"

Quaternion::Quaternion(double x, double y, double z, double w)
{
	this->x = x;
	this->y = y;
	this->z = z;
	this->w = w;
}

Quaternion::Quaternion(const Float3<double> &v, double w)
	: Quaternion(v.getX(), v.getY(), v.getZ(), w) { }

Quaternion::Quaternion(const Float4<double> &v)
	: Quaternion(v.getX(), v.getY(), v.getZ(), v.getW()) { }

Quaternion::Quaternion()
	: Quaternion(0.0, 0.0, 0.0, 0.0) { }

Quaternion::~Quaternion()
{

}

Quaternion Quaternion::identity()
{
	return Quaternion(0.0, 0.0, 0.0, 1.0);
}

Quaternion Quaternion::fromAxisAngle(const Float3<double> &v, double w)
{
	if (v.lengthSquared() < EPSILON)
	{
		return Quaternion::identity();
	}

	double halfW = w * 0.5;
	auto axis = v.normalized().scaledBy(std::sin(halfW));
	auto q = Quaternion(axis, std::cos(halfW));
	return q.normalized();
}

Quaternion Quaternion::fromAxisAngle(const Float4<double> &v)
{
	return Quaternion::fromAxisAngle(v.getX(), v.getY(), v.getZ(), v.getW());
}

Quaternion Quaternion::fromAxisAngle(double x, double y, double z, double w)
{
	return Quaternion(Float3<double>(x, y, z), w);
}

Quaternion Quaternion::operator *(const Quaternion &q) const
{
	auto left = Float3<double>(this->x, this->y, this->z);
	auto right = Float3<double>(q.x, q.y, q.z);
	auto axis = left.scaledBy(q.w) + right.scaledBy(this->w) + left.cross(right);
	double magnitude = (this->w * q.w) - left.dot(right);
	return Quaternion(axis, magnitude);
}

double Quaternion::getX() const
{
	return this->x;
}

double Quaternion::getY() const
{
	return this->y;
}

double Quaternion::getZ() const
{
	return this->z;
}

double Quaternion::getW() const
{
	return this->w;
}

Float3<double> Quaternion::getXYZ() const
{
	return Float3<double>(this->x, this->y, this->z);
}

Float4<double> Quaternion::getXYZW() const
{
	return Float4<double>(this->x, this->y, this->z, this->w);
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
