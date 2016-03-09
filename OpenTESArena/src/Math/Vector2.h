#ifndef VECTOR2_H
#define VECTOR2_H

#include <string>
#include <type_traits>

#include "Random.h"

template <typename T>
class Vector2
{
	static_assert(std::is_floating_point<T>::value, "Vector2<T> must be floating point.");
private:
	T x, y;

	static Random random;
public:
	Vector2(T x, T y);
	Vector2();
	~Vector2();

	static Vector2 randomDirection();
	static Vector2 randomPointInCircle(const Vector2 &center, T radius);
	static Vector2 randomPointInSquare(const Vector2 &center, T width, T height);

	Vector2 operator +(const Vector2 &v) const;
	Vector2 operator -(const Vector2 &v) const;
	Vector2 operator -() const;

	const T &getX() const;
	const T &getY() const;

	std::string toString() const;
	T lengthSquared() const;
	T length() const;
	Vector2 normalized() const;
	bool isNormalized() const;
	T dot(T m) const;
	T dot(const Vector2 &v) const;
	Vector2 scaledBy(T m) const;
	Vector2 scaledBy(const Vector2 &v) const;
	Vector2 lerp(const Vector2 &end, T percent) const;
	Vector2 slerp(const Vector2 &end, T percent) const;
	Vector2 leftPerp() const;
	Vector2 rightPerp() const;
};

// The template instantiations are at the end of the .cpp file.
typedef Vector2<float> Vector2f;
typedef Vector2<double> Vector2d;

#endif