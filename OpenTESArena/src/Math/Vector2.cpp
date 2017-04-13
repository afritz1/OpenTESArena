#include <cmath>

#include "Vector2.h"

#include "Constants.h"
#include "Random.h"

// -- Vector2i --

template <class T>
Vector2i<T>::Vector2i(T x, T y)
{
	this->x = x;
	this->y = y;
}

template <class T>
Vector2i<T>::Vector2i()
{
	this->x = static_cast<T>(0);
	this->y = static_cast<T>(0);
}

template <class T>
Vector2i<T>::~Vector2i()
{

}

template <class T>
T &Vector2i<T>::operator[](size_t index)
{
	return reinterpret_cast<T*>(&this->x)[index];
}

template <class T>
const T &Vector2i<T>::operator[](size_t index) const
{
	return reinterpret_cast<const T*>(&this->x)[index];
}

template <class T>
bool Vector2i<T>::operator==(const Vector2i<T> &v) const
{
	return (this->x == v.x) && (this->y == v.y);
}

template <class T>
bool Vector2i<T>::operator!=(const Vector2i<T> &v) const
{
	return (this->x != v.x) || (this->y != v.y);
}

template <class T>
Vector2i<T> Vector2i<T>::operator+(const Vector2i<T> &v) const
{
	return Vector2i<T>(this->x + v.x, this->y + v.y);
}

// operator-() is in the header.

template <class T>
Vector2i<T> Vector2i<T>::operator-(const Vector2i<T> &v) const
{
	return Vector2i<T>(this->x - v.x, this->y - v.y);
}

template <class T>
Vector2i<T> Vector2i<T>::operator*(T m) const
{
	return Vector2i<T>(this->x * m, this->y * m);
}

template <class T>
Vector2i<T> Vector2i<T>::operator*(const Vector2i<T> &v) const
{
	return Vector2i<T>(this->x * v.x, this->y * v.y);
}

template <class T>
Vector2i<T> Vector2i<T>::operator/(T m) const
{
	return Vector2i<T>(this->x / m, this->y / m);
}

template <class T>
Vector2i<T> Vector2i<T>::operator/(const Vector2i<T> &v) const
{
	return Vector2i<T>(this->x / v.x, this->y / v.y);
}

template <class T>
std::string Vector2i<T>::toString() const
{
	return std::to_string(this->x) + ", " + std::to_string(this->y);
}

// -- Vector2f --

template <class T>
const Vector2f<T> Vector2f<T>::UnitX =
	Vector2f<T>(static_cast<T>(1.0), static_cast<T>(0.0));

template <class T>
const Vector2f<T> Vector2f<T>::UnitY =
	Vector2f<T>(static_cast<T>(0.0), static_cast<T>(1.0));

template <class T>
Vector2f<T>::Vector2f(T x, T y)
{
	this->x = x;
	this->y = y;
}

template <class T>
Vector2f<T>::Vector2f()
{
	this->x = static_cast<T>(0.0);
	this->y = static_cast<T>(0.0);
}

template <class T>
Vector2f<T>::~Vector2f()
{

}

template <class T>
Vector2f<T> Vector2f<T>::randomDirection(Random &random)
{
	T x = static_cast<T>((2.0 * random.nextReal()) - 1.0);
	T y = static_cast<T>((2.0 * random.nextReal()) - 1.0);
	return Vector2f<T>(x, y).normalized();
}

template <class T>
Vector2f<T> Vector2f<T>::randomPointInCircle(const Vector2f<T> &center,
	T radius, Random &random)
{
	Vector2f<T> randPoint = Vector2f<T>::randomDirection(random) *
		(radius * static_cast<T>(random.nextReal()));
	return Vector2f<T>(center.x + randPoint.x, center.y + randPoint.y);
}

template <class T>
Vector2f<T> Vector2f<T>::randomPointInSquare(const Vector2f<T> &center,
	T width, T height, Random &random)
{
	return Vector2f<T>(
		center.x + (width * static_cast<T>(random.nextReal() - 0.50)),
		center.y + (height * static_cast<T>(random.nextReal() - 0.50)));
}

template <class T>
T &Vector2f<T>::operator[](size_t index)
{
	return reinterpret_cast<T*>(&this->x)[index];
}

template <class T>
const T &Vector2f<T>::operator[](size_t index) const
{
	return reinterpret_cast<const T*>(&this->x)[index];
}

template <class T>
bool Vector2f<T>::operator==(const Vector2f<T> &v) const
{
	return (this->x == v.x) && (this->y == v.y);
}

template <class T>
bool Vector2f<T>::operator!=(const Vector2f<T> &v) const
{
	return (this->x != v.x) || (this->y != v.y);
}

template <class T>
Vector2f<T> Vector2f<T>::operator+(const Vector2f<T> &v) const
{
	return Vector2f<T>(this->x + v.x, this->y + v.y);
}

template <class T>
Vector2f<T> Vector2f<T>::operator-() const
{
	return Vector2f<T>(-this->x, -this->y);
}

template <class T>
Vector2f<T> Vector2f<T>::operator-(const Vector2f<T> &v) const
{
	return Vector2f<T>(this->x - v.x, this->y - v.y);
}

template <class T>
Vector2f<T> Vector2f<T>::operator*(T m) const
{
	return Vector2f<T>(this->x * m, this->y * m);
}

template <class T>
Vector2f<T> Vector2f<T>::operator*(const Vector2f<T> &v) const
{
	return Vector2f<T>(this->x * v.x, this->y * v.y);
}

template <class T>
Vector2f<T> Vector2f<T>::operator/(T m) const
{
	return Vector2f<T>(this->x / m, this->y / m);
}

template <class T>
Vector2f<T> Vector2f<T>::operator/(const Vector2f<T> &v) const
{
	return Vector2f<T>(this->x / v.x, this->y / v.y);
}

template <class T>
std::string Vector2f<T>::toString() const
{
	return std::to_string(this->x) + ", " + std::to_string(this->y);
}

template <class T>
T Vector2f<T>::lengthSquared() const
{
	return (this->x * this->x) + (this->y * this->y);
}

template <class T>
T Vector2f<T>::length() const
{
	return std::sqrt(this->lengthSquared());
}

template <class T>
Vector2f<T> Vector2f<T>::normalized() const
{
	T lenRecip = static_cast<T>(1.0) / this->length();
	return Vector2f<T>(this->x * lenRecip, this->y * lenRecip);
}

template <class T>
bool Vector2f<T>::isNormalized() const
{
	return std::fabs(static_cast<T>(1.0) - this->length()) <
		static_cast<T>(EPSILON);
}

template <class T>
T Vector2f<T>::dot(const Vector2f<T> &v) const
{
	return (this->x * v.x) + (this->y * v.y);
}

template <class T>
Vector2f<T> Vector2f<T>::lerp(const Vector2f<T> &end, T percent) const
{
	return Vector2f<T>(
		this->x + ((end.x - this->x) * percent),
		this->y + ((end.y - this->y) * percent));
}

template <class T>
Vector2f<T> Vector2f<T>::slerp(const Vector2f<T> &end, T percent) const
{
	T theta = static_cast<T>(std::acos(this->dot(end) / (this->length() * end.length())));
	T sinThetaRecip = static_cast<T>(1.0 / std::sin(theta));
	T beginScale = static_cast<T>(std::sin((1.0 - percent) * theta) * sinThetaRecip);
	T endScale = std::sin(percent * theta) * sinThetaRecip;
	return ((*this) * beginScale) + (end * endScale);
}

template <class T>
Vector2f<T> Vector2f<T>::leftPerp() const
{
	return Vector2f<T>(-this->y, this->x);
}

template <class T>
Vector2f<T> Vector2f<T>::rightPerp() const
{
	return Vector2f<T>(this->y, this->x);
}

// Template instantiations.
template class Vector2i<char>;
template class Vector2i<unsigned char>;
template class Vector2i<short>;
template class Vector2i<unsigned short>;
template class Vector2i<int>;
template class Vector2i<unsigned int>;

template class Vector2f<float>;
template class Vector2f<double>;
