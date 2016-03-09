#ifndef QUATERNION_H
#define QUATERNION_H

#include <string>

template<typename T>
class Vector3;

template<typename T>
class Vector4;

class Quaternion
{
private:
	double x, y, z, w;
public:
	Quaternion(double x, double y, double z, double w);
	Quaternion(const Vector3<double> &v, double w);
	Quaternion(const Vector4<double> &v);
	Quaternion();
	~Quaternion();

	static Quaternion identity();
	static Quaternion fromAxisAngle(const Vector3<double> &v, double w);
	static Quaternion fromAxisAngle(const Vector4<double> &v);
	static Quaternion fromAxisAngle(double x, double y, double z, double w);

	Quaternion operator *(const Quaternion &q) const;

	const double &getX() const;
	const double &getY() const;
	const double &getZ() const;
	const double &getW() const;
	Vector3<double> getXYZ() const;
	Vector4<double> getXYZW() const;

	std::string toString() const;
	double length() const;
	Quaternion normalized() const;
};

#endif