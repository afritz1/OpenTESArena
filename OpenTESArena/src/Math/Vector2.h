#ifndef VECTOR2_H
#define VECTOR2_H

#include <cstddef>
#include <cstdlib> // std::abs.
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

class Random;

template <typename T>
class Vector2i
{
public:
	static_assert(std::is_integral<T>::value);

	static const Vector2i<T> Zero;

	T x, y;

	Vector2i(T x, T y);
	Vector2i();

	T &operator[](size_t index);
	const T &operator[](size_t index) const;
	bool operator==(const Vector2i<T> &v) const;
	bool operator!=(const Vector2i<T> &v) const;
	Vector2i<T> operator+(const Vector2i<T> &v) const;
	
	// Only signed integers can use negation.
	template <typename C = T>
	typename std::enable_if_t<std::is_signed<C>::value, Vector2i<T>> operator-() const
	{
		return Vector2i<T>(-this->x, -this->y);
	}

	Vector2i<T> operator-(const Vector2i<T> &v) const;
	Vector2i<T> operator*(T m) const;
	Vector2i<T> operator*(const Vector2i<T> &v) const;
	Vector2i<T> operator/(T m) const;
	Vector2i<T> operator/(const Vector2i<T> &v) const;

	std::string toString() const;
};

template <typename T>
class Vector2f
{
public:
	static_assert(std::is_floating_point<T>::value);

	static const Vector2f<T> Zero;
	static const Vector2f<T> UnitX;
	static const Vector2f<T> UnitY;

	T x, y;

	Vector2f(T x, T y);
	Vector2f();

	static Vector2f<T> randomDirection(Random &random);
	static Vector2f<T> randomPointInCircle(const Vector2f<T> &center, T radius, Random &random);
	static Vector2f<T> randomPointInSquare(const Vector2f<T> &center, T width, T height,
		Random &random);

	T &operator[](size_t index);
	const T &operator[](size_t index) const;
	bool operator==(const Vector2f<T> &v) const;
	bool operator!=(const Vector2f<T> &v) const;
	Vector2f<T> operator+(const Vector2f<T> &v) const;
	Vector2f<T> operator-() const;
	Vector2f<T> operator-(const Vector2f<T> &v) const;
	Vector2f<T> operator*(T m) const;
	Vector2f<T> operator*(const Vector2f<T> &v) const;
	Vector2f<T> operator/(T m) const;
	Vector2f<T> operator/(const Vector2f<T> &v) const;

	std::string toString() const;
	T lengthSquared() const;
	T length() const;
	Vector2f<T> normalized() const;
	bool isNormalized() const;
	T dot(const Vector2f<T> &v) const;
	Vector2f<T> lerp(const Vector2f<T> &end, T percent) const;
	Vector2f<T> slerp(const Vector2f<T> &end, T percent) const;
	Vector2f<T> leftPerp() const;
	Vector2f<T> rightPerp() const;
};

// Unit vector definitions (can't be in .cpp file on Clang).
template <typename T>
const Vector2i<T> Vector2i<T>::Zero(static_cast<T>(0), static_cast<T>(0));

template <typename T>
const Vector2f<T> Vector2f<T>::Zero(static_cast<T>(0.0), static_cast<T>(0.0));

template <typename T>
const Vector2f<T> Vector2f<T>::UnitX(static_cast<T>(1.0), static_cast<T>(0.0));

template <typename T>
const Vector2f<T> Vector2f<T>::UnitY(static_cast<T>(0.0), static_cast<T>(1.0));

// The template instantiations are at the end of the .cpp file.
using Char2 = Vector2i<char>;
using Uchar2 = Vector2i<unsigned char>;
using Short2 = Vector2i<short>;
using Ushort2 = Vector2i<unsigned short>;
using Int2 = Vector2i<int>;
using Uint2 = Vector2i<unsigned int>;

using Float2 = Vector2f<float>;
using Double2 = Vector2f<double>;

// Hash definition for unordered_map<Int2, ...>.
namespace std
{
	template <>
	struct hash<Int2>
	{
		size_t operator()(const Int2 &v) const
		{
			// Multiply with a prime number before xor'ing.
			return static_cast<size_t>(v.x ^ (v.y * 41));
		}
	};
}

#endif
