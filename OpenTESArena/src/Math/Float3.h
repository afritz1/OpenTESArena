#ifndef FLOAT3_H
#define FLOAT3_H

#include <string>
#include <type_traits>

class Color;
class Random;

template <typename T>
class Float3
{
	static_assert(std::is_floating_point<T>::value, "Float3<T> must be floating point.");
private:
	T x, y, z;
public:
	Float3(T x, T y, T z);
	Float3();
	~Float3();

	static Float3 randomDirection(Random &random);
	static Float3 randomPointInSphere(const Float3 &center, T radius, Random &random);
	static Float3 randomPointInCuboid(const Float3 &center, T width, T height, T depth,
		Random &random);
	static Float3 fromRGB(unsigned int rgb);
	static Float3 fromColor(const Color &c);

	Float3 operator +(const Float3 &v) const;
	Float3 operator -(const Float3 &v) const;
	Float3 operator -() const;
	Float3 operator *(T m) const;
	Float3 operator *(const Float3 &v) const;

	T getX() const;
	T getY() const;
	T getZ() const;

	std::string toString() const;
	unsigned int toRGB() const;
	Color toColor() const;
	T lengthSquared() const;
	T length() const;
	Float3 normalized() const;
	bool isNormalized() const;
	T dot(T m) const;
	T dot(const Float3 &v) const;
	Float3 cross(const Float3 &v) const;
	Float3 reflect(const Float3 &normal) const;
	Float3 lerp(const Float3 &end, T percent) const;
	Float3 slerp(const Float3 &end, T percent) const;
	Float3 clamped(T low, T high) const;
	Float3 clamped() const;
	Float3 componentMin(const Float3 &v) const;
	Float3 componentMax(const Float3 &v) const;
};

// The template instantiations are at the end of the .cpp file.
typedef Float3<float> Float3f;
typedef Float3<double> Float3d;

#endif
