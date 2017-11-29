#ifndef VECTOR4_H
#define VECTOR4_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>

#include "Vector3.h"

template <class T>
class Vector4i
{
public:
	static_assert(std::is_integral<T>::value, "Vector4i<T> must be integral type.");

	T x, y, z, w;

	Vector4i(T x, T y, T z, T w);
	Vector4i();
	~Vector4i();

	T &operator[](size_t index);
	const T &operator[](size_t index) const;
	bool operator==(const Vector4i<T> &v) const;
	bool operator!=(const Vector4i<T> &v) const;
	Vector4i<T> operator+(const Vector4i<T> &v) const;

	// Only signed integers can use negation.
	template <class C = T>
	typename std::enable_if<std::is_signed<C>::value, Vector4i<T>>::type operator-() const
	{
		return Vector4i<T>(-this->x, -this->y, -this->z, -this->w);
	}

	Vector4i<T> operator-(const Vector4i<T> &v) const;
	Vector4i<T> operator*(T m) const;
	Vector4i<T> operator*(const Vector4i<T> &v) const;
	Vector4i<T> operator/(T m) const;
	Vector4i<T> operator/(const Vector4i<T> &v) const;

	std::string toString() const;
};

template <class T>
class Vector4f
{
public:
	static_assert(std::is_floating_point<T>::value, "Vector4f<T> must be floating-point type.");
	
	static const Vector4f<T> UnitX;
	static const Vector4f<T> UnitY;
	static const Vector4f<T> UnitZ;
	static const Vector4f<T> UnitW;

	T x, y, z, w;

	Vector4f(T x, T y, T z, T w);
	Vector4f(const Vector3f<T> &xyz, T w);
	Vector4f();
	~Vector4f();

	static Vector4f<T> fromARGB(uint32_t argb);
	static Vector4f<T> fromRGBA(uint32_t rgba);

	T &operator[](size_t index);
	const T &operator[](size_t index) const;
	bool operator==(const Vector4f<T> &v) const;
	bool operator!=(const Vector4f<T> &v) const;
	Vector4f<T> operator+(const Vector4f<T> &v) const;
	Vector4f<T> operator-() const;
	Vector4f<T> operator-(const Vector4f<T> &v) const;
	Vector4f<T> operator*(T m) const;
	Vector4f<T> operator*(const Vector4f<T> &v) const;
	Vector4f<T> operator/(T m) const;
	Vector4f<T> operator/(const Vector4f<T> &v) const;

	std::string toString() const;
	uint32_t toARGB() const;
	uint32_t toRGBA() const;
	T lengthSquared() const;
	T length() const;
	Vector4f<T> lerp(const Vector4f<T> &end, T percent) const;
	Vector4f<T> clamped(T low, T high) const;
	Vector4f<T> clamped() const;
};

// The template instantiations are at the end of the .cpp file.
typedef Vector4i<char> Char4;
typedef Vector4i<unsigned char> Uchar4;
typedef Vector4i<short> Short4;
typedef Vector4i<unsigned short> Ushort4;
typedef Vector4i<int> Int4;
typedef Vector4i<unsigned int> Uint4;

typedef Vector4f<float> Float4;
typedef Vector4f<double> Double4;

#endif
