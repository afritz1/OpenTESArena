#ifndef FLOAT2_H
#define FLOAT2_H

#include <string>
#include <type_traits>

#include "Random.h"

template <typename T>
class Float2
{
	static_assert(std::is_floating_point<T>::value, "Float2<T> must be floating point.");
private:
	T x, y;

	static Random random;
public:
	Float2(T x, T y);
	Float2();
	~Float2();

	static Float2 randomDirection();
	static Float2 randomPointInCircle(const Float2 &center, T radius);
	static Float2 randomPointInSquare(const Float2 &center, T width, T height);

	Float2 operator +(const Float2 &v) const;
	Float2 operator -(const Float2 &v) const;
	Float2 operator -() const;

	const T &getX() const;
	const T &getY() const;

	std::string toString() const;
	T lengthSquared() const;
	T length() const;
	Float2 normalized() const;
	bool isNormalized() const;
	T dot(T m) const;
	T dot(const Float2 &v) const;
	Float2 scaledBy(T m) const;
	Float2 scaledBy(const Float2 &v) const;
	Float2 lerp(const Float2 &end, T percent) const;
	Float2 slerp(const Float2 &end, T percent) const;
	Float2 leftPerp() const;
	Float2 rightPerp() const;
};

// The template instantiations are at the end of the .cpp file.
typedef Float2<float> Float2f;
typedef Float2<double> Float2d;

#endif