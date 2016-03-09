#ifndef VECTOR4_H
#define VECTOR4_H

#include <string>
#include <type_traits>

class Color;

template <typename T>
class Vector3;

template <typename T>
class Vector4
{
	static_assert(std::is_floating_point<T>::value, "Vector4<T> must be floating point.");
private:
	T x, y, z, w;
public:
	Vector4(T x, T y, T z, T w);
	Vector4(const Vector3<T> &v, T w);
	Vector4(const Vector3<T> &v);
	Vector4();
	~Vector4();

	static Vector4 fromARGB(unsigned int argb);
	static Vector4 fromColor(const Color &c);

	Vector4 operator +(const Vector4 &v) const;
	Vector4 operator -(const Vector4 &v) const;
	Vector4 operator -() const;

	const T &getX() const;
	const T &getY() const;
	const T &getZ() const;
	const T &getW() const;
	Vector3<T> getXYZ() const;

	std::string toString() const;
	unsigned int toARGB() const;
	Color toColor() const;
};

// The template instantiations are at the end of the .cpp file.
typedef Vector4<float> Vector4f;
typedef Vector4<double> Vector4d;

#endif