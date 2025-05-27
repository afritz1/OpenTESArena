#ifndef VECTOR4_H
#define VECTOR4_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>

#include "Vector3.h"

template<typename T>
struct Vector4i
{
	static_assert(std::is_integral<T>::value);

	T x, y, z, w;

	constexpr Vector4i(T x, T y, T z, T w)
	{
		this->x = x;
		this->y = y;
		this->z = z;
		this->w = w;
	}

	constexpr Vector4i()
	{
		this->x = static_cast<T>(0);
		this->y = static_cast<T>(0);
		this->z = static_cast<T>(0);
		this->w = static_cast<T>(0);
	}

	static const Vector4i<T> Zero;
	static const Vector4i<T> UnitX;
	static const Vector4i<T> UnitY;
	static const Vector4i<T> UnitZ;
	static const Vector4i<T> UnitW;

	T &operator[](size_t index);
	const T &operator[](size_t index) const;
	bool operator==(const Vector4i<T> &v) const;
	bool operator!=(const Vector4i<T> &v) const;
	Vector4i<T> operator+(const Vector4i<T> &v) const;

	// Only signed integers can use negation.
	template<typename C = T>
	typename std::enable_if_t<std::is_signed<C>::value, Vector4i<T>> operator-() const
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

template<typename T>
struct Vector4f
{
	static_assert(std::is_floating_point<T>::value);

	T x, y, z, w;

	constexpr Vector4f(T x, T y, T z, T w)
	{
		this->x = x;
		this->y = y;
		this->z = z;
		this->w = w;
	}

	constexpr Vector4f(const Vector3f<T> &xyz, T w)
	{
		this->x = xyz.x;
		this->y = xyz.y;
		this->z = xyz.z;
		this->w = w;
	}

	constexpr Vector4f()
	{
		this->x = static_cast<T>(0.0);
		this->y = static_cast<T>(0.0);
		this->z = static_cast<T>(0.0);
		this->w = static_cast<T>(0.0);
	}

	static const Vector4f<T> Zero;
	static const Vector4f<T> UnitX;
	static const Vector4f<T> UnitY;
	static const Vector4f<T> UnitZ;
	static const Vector4f<T> UnitW;

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

// Unit vector definitions (can't be in .cpp file on Clang).
template<typename T>
const Vector4i<T> Vector4i<T>::Zero(static_cast<T>(0), static_cast<T>(0), static_cast<T>(0), static_cast<T>(0));
template<typename T>
const Vector4i<T> Vector4i<T>::UnitX(static_cast<T>(1), static_cast<T>(0), static_cast<T>(0), static_cast<T>(0));
template<typename T>
const Vector4i<T> Vector4i<T>::UnitY(static_cast<T>(0), static_cast<T>(1), static_cast<T>(0), static_cast<T>(0));
template<typename T>
const Vector4i<T> Vector4i<T>::UnitZ(static_cast<T>(0), static_cast<T>(0), static_cast<T>(1), static_cast<T>(0));
template<typename T>
const Vector4i<T> Vector4i<T>::UnitW(static_cast<T>(0), static_cast<T>(0), static_cast<T>(0), static_cast<T>(1));

template<typename T>
const Vector4f<T> Vector4f<T>::Zero(static_cast<T>(0.0), static_cast<T>(0.0), static_cast<T>(0.0), static_cast<T>(0.0));
template<typename T>
const Vector4f<T> Vector4f<T>::UnitX(static_cast<T>(1.0), static_cast<T>(0.0), static_cast<T>(0.0), static_cast<T>(0.0));
template<typename T>
const Vector4f<T> Vector4f<T>::UnitY(static_cast<T>(0.0), static_cast<T>(1.0), static_cast<T>(0.0), static_cast<T>(0.0));
template<typename T>
const Vector4f<T> Vector4f<T>::UnitZ(static_cast<T>(0.0), static_cast<T>(0.0), static_cast<T>(1.0), static_cast<T>(0.0));
template<typename T>
const Vector4f<T> Vector4f<T>::UnitW(static_cast<T>(0.0), static_cast<T>(0.0), static_cast<T>(0.0), static_cast<T>(1.0));

// The template instantiations are at the end of the .cpp file.
using Char4 = Vector4i<char>;
using Uchar4 = Vector4i<unsigned char>;
using Short4 = Vector4i<short>;
using Ushort4 = Vector4i<unsigned short>;
using Int4 = Vector4i<int>;
using Uint4 = Vector4i<unsigned int>;

using Float4 = Vector4f<float>;
using Double4 = Vector4f<double>;

#endif
