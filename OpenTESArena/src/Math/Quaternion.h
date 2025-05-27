#ifndef QUATERNION_H
#define QUATERNION_H

#include <string>

#include "Vector3.h"
#include "Vector4.h"

struct Quaternion
{
	double x, y, z, w;

	Quaternion(double x, double y, double z, double w);
	Quaternion(const Double3 &v, double w);
	Quaternion(const Double4 &v);
	Quaternion();

	static Quaternion identity();
	static Quaternion fromAxisAngle(const Double3 &v, double w);
	static Quaternion fromAxisAngle(const Double4 &v);
	static Quaternion fromAxisAngle(double x, double y, double z, double w);

	Quaternion operator*(const Quaternion &q) const;
	
	std::string toString() const;
	double length() const;
	Quaternion normalized() const;
};

#endif
