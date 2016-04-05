#ifndef QUATERNION_H
#define QUATERNION_H

#include <string>

template<typename T>
class Float3;

template<typename T>
class Float4;

class Quaternion
{
private:
	double x, y, z, w;
public:
	Quaternion(double x, double y, double z, double w);
	Quaternion(const Float3<double> &v, double w);
	Quaternion(const Float4<double> &v);
	Quaternion();
	~Quaternion();

	static Quaternion identity();
	static Quaternion fromAxisAngle(const Float3<double> &v, double w);
	static Quaternion fromAxisAngle(const Float4<double> &v);
	static Quaternion fromAxisAngle(double x, double y, double z, double w);

	Quaternion operator *(const Quaternion &q) const;

	const double &getX() const;
	const double &getY() const;
	const double &getZ() const;
	const double &getW() const;
	Float3<double> getXYZ() const;
	Float4<double> getXYZW() const;

	std::string toString() const;
	double length() const;
	Quaternion normalized() const;
};

#endif