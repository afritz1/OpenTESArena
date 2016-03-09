#ifndef VECTOR3_H
#define VECTOR3_H

#include <string>
#include <type_traits>

#include "Random.h"

class Color;

template <typename T>
class Vector3
{
	static_assert(std::is_floating_point<T>::value, "Vector3<T> must be floating point.");
private:
	T x, y, z;

	static Random random;
public:
	Vector3(T x, T y, T z);
	Vector3();
	~Vector3();

	static Vector3 randomDirection();
	static Vector3 randomPointInSphere(const Vector3 &center, T radius);
	static Vector3 randomPointInCuboid(const Vector3 &center, T width, T height, T depth);
	static Vector3 fromRGB(unsigned int rgb);
	static Vector3 fromColor(const Color &c);

	Vector3 operator +(const Vector3 &v) const;
	Vector3 operator -(const Vector3 &v) const;
	Vector3 operator -() const;

	const T &getX() const;
	const T &getY() const;
	const T &getZ() const;

	std::string toString() const;
	unsigned int toRGB() const;
	Color toColor() const;
	T lengthSquared() const;
	T length() const;
	Vector3 normalized() const;
	bool isNormalized() const;
	T dot(T m) const;
	T dot(const Vector3 &v) const;
	Vector3 cross(const Vector3 &v) const;
	Vector3 reflect(const Vector3 &normal) const;
	Vector3 scaledBy(T m) const;
	Vector3 scaledBy(const Vector3 &v) const;
	Vector3 lerp(const Vector3 &end, T percent) const;
	Vector3 slerp(const Vector3 &end, T percent) const;
	Vector3 clamped(T low, T high) const;
	Vector3 clamped() const;
	Vector3 componentMin(const Vector3 &v) const;
	Vector3 componentMax(const Vector3 &v) const;
};

// The template instantiations are at the end of the .cpp file.
typedef Vector3<float> Vector3f;
typedef Vector3<double> Vector3d;

#endif