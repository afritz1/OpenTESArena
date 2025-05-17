#include <cmath>
#include <cstdio>

#include "Constants.h"
#include "Random.h"
#include "Vector2.h"

// -- Vector2i --

template<typename T>
T &Vector2i<T>::operator[](size_t index)
{
	return reinterpret_cast<T*>(&this->x)[index];
}

template<typename T>
const T &Vector2i<T>::operator[](size_t index) const
{
	return reinterpret_cast<const T*>(&this->x)[index];
}

template<typename T>
bool Vector2i<T>::operator==(const Vector2i<T> &v) const
{
	return (this->x == v.x) && (this->y == v.y);
}

template<typename T>
bool Vector2i<T>::operator!=(const Vector2i<T> &v) const
{
	return (this->x != v.x) || (this->y != v.y);
}

template<typename T>
Vector2i<T> Vector2i<T>::operator+(const Vector2i<T> &v) const
{
	return Vector2i<T>(this->x + v.x, this->y + v.y);
}

// operator-() is in the header.

template<typename T>
Vector2i<T> Vector2i<T>::operator-(const Vector2i<T> &v) const
{
	return Vector2i<T>(this->x - v.x, this->y - v.y);
}

template<typename T>
Vector2i<T> Vector2i<T>::operator*(T m) const
{
	return Vector2i<T>(this->x * m, this->y * m);
}

template<typename T>
Vector2i<T> Vector2i<T>::operator*(const Vector2i<T> &v) const
{
	return Vector2i<T>(this->x * v.x, this->y * v.y);
}

template<typename T>
Vector2i<T> Vector2i<T>::operator/(T m) const
{
	return Vector2i<T>(this->x / m, this->y / m);
}

template<typename T>
Vector2i<T> Vector2i<T>::operator/(const Vector2i<T> &v) const
{
	return Vector2i<T>(this->x / v.x, this->y / v.y);
}

template<typename T>
std::string Vector2i<T>::toString() const
{
	char buffer[64];
	std::snprintf(buffer, std::size(buffer), "%d, %d", this->x, this->y);
	return std::string(buffer);
}

// -- Vector2f --

template<typename T>
Vector2f<T> Vector2f<T>::randomDirection(Random &random)
{
	const T x = static_cast<T>((2.0 * random.nextReal()) - 1.0);
	const T y = static_cast<T>((2.0 * random.nextReal()) - 1.0);
	return Vector2f<T>(x, y).normalized();
}

template<typename T>
Vector2f<T> Vector2f<T>::randomPointInCircle(const Vector2f<T> &center,
	T radius, Random &random)
{
	const Vector2f<T> randPoint = Vector2f<T>::randomDirection(random) * (radius * static_cast<T>(random.nextReal()));
	return Vector2f<T>(center.x + randPoint.x, center.y + randPoint.y);
}

template<typename T>
Vector2f<T> Vector2f<T>::randomPointInSquare(const Vector2f<T> &center,
	T width, T height, Random &random)
{
	return Vector2f<T>(
		center.x + (width * static_cast<T>(random.nextReal() - 0.50)),
		center.y + (height * static_cast<T>(random.nextReal() - 0.50)));
}

template<typename T>
T &Vector2f<T>::operator[](size_t index)
{
	return reinterpret_cast<T*>(&this->x)[index];
}

template<typename T>
const T &Vector2f<T>::operator[](size_t index) const
{
	return reinterpret_cast<const T*>(&this->x)[index];
}

template<typename T>
bool Vector2f<T>::operator==(const Vector2f<T> &v) const
{
	return (this->x == v.x) && (this->y == v.y);
}

template<typename T>
bool Vector2f<T>::operator!=(const Vector2f<T> &v) const
{
	return (this->x != v.x) || (this->y != v.y);
}

template<typename T>
Vector2f<T> Vector2f<T>::operator+(const Vector2f<T> &v) const
{
	return Vector2f<T>(this->x + v.x, this->y + v.y);
}

template<typename T>
Vector2f<T> Vector2f<T>::operator-() const
{
	return Vector2f<T>(-this->x, -this->y);
}

template<typename T>
Vector2f<T> Vector2f<T>::operator-(const Vector2f<T> &v) const
{
	return Vector2f<T>(this->x - v.x, this->y - v.y);
}

template<typename T>
Vector2f<T> Vector2f<T>::operator*(T m) const
{
	return Vector2f<T>(this->x * m, this->y * m);
}

template<typename T>
Vector2f<T> Vector2f<T>::operator*(const Vector2f<T> &v) const
{
	return Vector2f<T>(this->x * v.x, this->y * v.y);
}

template<typename T>
Vector2f<T> Vector2f<T>::operator/(T m) const
{
	return Vector2f<T>(this->x / m, this->y / m);
}

template<typename T>
Vector2f<T> Vector2f<T>::operator/(const Vector2f<T> &v) const
{
	return Vector2f<T>(this->x / v.x, this->y / v.y);
}

template<typename T>
std::string Vector2f<T>::toString() const
{
	char buffer[64];
	std::snprintf(buffer, std::size(buffer), "%.2f, %.2f", this->x, this->y);
	return std::string(buffer);
}

template<typename T>
T Vector2f<T>::lengthSquared() const
{
	return (this->x * this->x) + (this->y * this->y);
}

template<typename T>
T Vector2f<T>::length() const
{
	return std::sqrt(this->lengthSquared());
}

template<typename T>
Vector2f<T> Vector2f<T>::normalized() const
{
	const T lenRecip = static_cast<T>(1.0) / this->length();
	return Vector2f<T>(this->x * lenRecip, this->y * lenRecip);
}

template<typename T>
bool Vector2f<T>::isNormalized() const
{
	return std::fabs(static_cast<T>(1.0) - this->length()) < static_cast<T>(Constants::Epsilon);
}

template<typename T>
T Vector2f<T>::dot(const Vector2f<T> &v) const
{
	return (this->x * v.x) + (this->y * v.y);
}

template<typename T>
T Vector2f<T>::cross(const Vector2f<T> &v) const
{
	return (this->x * v.y) - (this->y * v.x);
}

template<typename T>
Vector2f<T> Vector2f<T>::lerp(const Vector2f<T> &end, T percent) const
{
	return Vector2f<T>(
		this->x + ((end.x - this->x) * percent),
		this->y + ((end.y - this->y) * percent));
}

template<typename T>
Vector2f<T> Vector2f<T>::slerp(const Vector2f<T> &end, T percent) const
{
	const T theta = static_cast<T>(std::acos(this->dot(end) / (this->length() * end.length())));
	const T sinThetaRecip = static_cast<T>(1.0 / std::sin(theta));
	const T beginScale = static_cast<T>(std::sin((1.0 - percent) * theta) * sinThetaRecip);
	const T endScale = std::sin(percent * theta) * sinThetaRecip;
	return ((*this) * beginScale) + (end * endScale);
}

template<typename T>
Vector2f<T> Vector2f<T>::leftPerp() const
{
	return Vector2f<T>(-this->y, this->x);
}

template<typename T>
Vector2f<T> Vector2f<T>::rightPerp() const
{
	return Vector2f<T>(this->y, -this->x);
}

// Template instantiations.
template struct Vector2i<char>;
template struct Vector2i<unsigned char>;
template struct Vector2i<short>;
template struct Vector2i<unsigned short>;
template struct Vector2i<int>;
template struct Vector2i<unsigned int>;

template struct Vector2f<float>;
template struct Vector2f<double>;
