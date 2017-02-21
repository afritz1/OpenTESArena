#ifndef VECTOR2_H
#define VECTOR2_H

#include <cstddef>
#include <string>
#include <type_traits>

class Random;

template <class T>
class Vector2i
{
public:
	static_assert(std::is_integral<T>::value, "Vector2i<T> must be integral type.");

	T x, y;

	Vector2i(T x, T y);
	Vector2i();
	~Vector2i();

	T &operator[](size_t index);
	const T &operator[](size_t index) const;
	bool operator==(const Vector2i<T> &v) const;
	bool operator!=(const Vector2i<T> &v) const;
	Vector2i<T> operator+(const Vector2i<T> &v) const;
	
	// Only signed integers can use negation.
	template <class C = T>
	typename std::enable_if<std::is_signed<C>::value, Vector2i<T>>::type operator-() const
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

template <class T>
class Vector2f
{
public:
	static_assert(std::is_floating_point<T>::value, "Vector2f<T> must be floating-point type.");

	static const Vector2f<T> UnitX;
	static const Vector2f<T> UnitY;

	T x, y;

	Vector2f(T x, T y);
	Vector2f();
	~Vector2f();

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

// The template instantiations are at the end of the .cpp file.
typedef Vector2i<char> Char2;
typedef Vector2i<unsigned char> Uchar2;
typedef Vector2i<short> Short2;
typedef Vector2i<unsigned short> Ushort2;
typedef Vector2i<int> Int2;
typedef Vector2i<unsigned int> Uint2;
typedef Vector2i<size_t> Size_t2;

typedef Vector2f<float> Float2;
typedef Vector2f<double> Double2;

#endif
