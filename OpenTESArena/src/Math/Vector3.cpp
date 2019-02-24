#include <cmath>

#include "Constants.h"
#include "Random.h"
#include "Vector3.h"

// -- Vector3i --

template <class T>
Vector3i<T>::Vector3i(T x, T y, T z)
{
	this->x = x;
	this->y = y;
	this->z = z;
}

template <class T>
Vector3i<T>::Vector3i()
{
	this->x = static_cast<T>(0);
	this->y = static_cast<T>(0);
	this->z = static_cast<T>(0);
}

template <class T>
T &Vector3i<T>::operator[](size_t index)
{
	return reinterpret_cast<T*>(&this->x)[index];
}

template <class T>
const T &Vector3i<T>::operator[](size_t index) const
{
	return reinterpret_cast<const T*>(&this->x)[index];
}

template <class T>
bool Vector3i<T>::operator==(const Vector3i<T> &v) const
{
	return (this->x == v.x) && (this->y == v.y) && (this->z == v.z);
}

template <class T>
bool Vector3i<T>::operator!=(const Vector3i<T> &v) const
{
	return (this->x != v.x) || (this->y != v.y) || (this->z != v.z);
}

template <class T>
Vector3i<T> Vector3i<T>::operator+(const Vector3i<T> &v) const
{
	return Vector3i<T>(this->x + v.x, this->y + v.y, this->z + v.z);
}

// operator-() is in the header.

template <class T>
Vector3i<T> Vector3i<T>::operator-(const Vector3i<T> &v) const
{
	return Vector3i<T>(this->x - v.x, this->y - v.y, this->z - v.z);
}

template <class T>
Vector3i<T> Vector3i<T>::operator*(T m) const
{
	return Vector3i<T>(this->x * m, this->y * m, this->z * m);
}

template <class T>
Vector3i<T> Vector3i<T>::operator*(const Vector3i<T> &v) const
{
	return Vector3i<T>(this->x * v.x, this->y * v.y, this->z * v.z);
}

template <class T>
Vector3i<T> Vector3i<T>::operator/(T m) const
{
	return Vector3i<T>(this->x / m, this->y / m, this->z / m);
}

template <class T>
Vector3i<T> Vector3i<T>::operator/(const Vector3i<T> &v) const
{
	return Vector3i<T>(this->x / v.x, this->y / v.y, this->z / v.z);
}

template <class T>
std::string Vector3i<T>::toString() const
{
	return std::to_string(this->x) + ", " + std::to_string(this->y) + ", " +
		std::to_string(this->z);
}

// -- Vector3f --

template <class T>
Vector3f<T>::Vector3f(T x, T y, T z)
{
	this->x = x;
	this->y = y;
	this->z = z;
}

template <class T>
Vector3f<T>::Vector3f()
{
	this->x = static_cast<T>(0.0);
	this->y = static_cast<T>(0.0);
	this->z = static_cast<T>(0.0);
}

template<>
Vector3f<float>::Vector3f(const Vector3f<double>& old)
{
	this->x = static_cast<float>(old.x);
	this->y = static_cast<float>(old.y);
	this->z = static_cast<float>(old.z);
}

template<>
Vector3f<double>::Vector3f(const Vector3f<float>& old)
{
	this->x = static_cast<double>(old.x);
	this->y = static_cast<double>(old.y);
	this->z = static_cast<double>(old.z);
}

template<>
Vector3f<double>::Vector3f(const Vector3f<double>& old)
{
	this->x = old.x;
	this->y = old.y;
	this->z = old.z;
}

template<>
Vector3f<float>::Vector3f(const Vector3f<float>& old)
{
	this->x = old.x;
	this->y = old.y;
	this->z = old.z;
}

template <class T>
Vector3f<T> Vector3f<T>::randomDirection(Random &random)
{
	T x = static_cast<T>((2.0 * random.nextReal()) - 1.0);
	T y = static_cast<T>((2.0 * random.nextReal()) - 1.0);
	T z = static_cast<T>((2.0 * random.nextReal()) - 1.0);
	return Vector3f<T>(x, y, z).normalized();
}

template <class T>
Vector3f<T> Vector3f<T>::randomPointInSphere(const Vector3f<T> &center,
	T radius, Random &random)
{
	Vector3f<T> randPoint = Vector3f<T>::randomDirection(random) *
		(radius * static_cast<T>(random.nextReal()));
	return Vector3f<T>(
		center.x + randPoint.x,
		center.y + randPoint.y,
		center.z + randPoint.z);
}

template <class T>
Vector3f<T> Vector3f<T>::randomPointInCuboid(const Vector3f<T> &center,
	T width, T height, T depth, Random &random)
{
	return Vector3f<T>(
		center.x + (width * static_cast<T>(random.nextReal() - 0.50)),
		center.y + (height * static_cast<T>(random.nextReal() - 0.50)),
		center.z + (depth * static_cast<T>(random.nextReal() - 0.50)));
}

template <class T>
Vector3f<T> Vector3f<T>::fromRGB(uint32_t rgb)
{
	return Vector3f<T>(
		static_cast<T>(static_cast<uint8_t>(rgb >> 16) / 255.0),
		static_cast<T>(static_cast<uint8_t>(rgb >> 8) / 255.0),
		static_cast<T>(static_cast<uint8_t>(rgb) / 255.0));
}

template <class T>
T &Vector3f<T>::operator[](size_t index)
{
	return reinterpret_cast<T*>(&this->x)[index];
}

template <class T>
const T &Vector3f<T>::operator[](size_t index) const
{
	return reinterpret_cast<const T*>(&this->x)[index];
}

template <class T>
bool Vector3f<T>::operator==(const Vector3f<T> &v) const
{
	return (this->x == v.x) && (this->y == v.y) && (this->z == v.z);
}

template <class T>
bool Vector3f<T>::operator!=(const Vector3f<T> &v) const
{
	return (this->x != v.x) || (this->y != v.y) || (this->z != v.z);
}

template <class T>
Vector3f<T> Vector3f<T>::operator+(const Vector3f<T> &v) const
{
	return Vector3f<T>(this->x + v.x, this->y + v.y, this->z + v.z);
}

template <class T>
Vector3f<T> Vector3f<T>::operator-() const
{
	return Vector3f<T>(-this->x, -this->y, -this->z);
}

template <class T>
Vector3f<T> Vector3f<T>::operator-(const Vector3f<T> &v) const
{
	return Vector3f<T>(this->x - v.x, this->y - v.y, this->z - v.z);
}

template <class T>
Vector3f<T> Vector3f<T>::operator*(T m) const
{
	return Vector3f<T>(this->x * m, this->y * m, this->z * m);
}

template <class T>
Vector3f<T> Vector3f<T>::operator*(const Vector3f<T> &v) const
{
	return Vector3f<T>(this->x * v.x, this->y * v.y, this->z * v.z);
}

template <class T>
Vector3f<T> Vector3f<T>::operator/(T m) const
{
	return Vector3f<T>(this->x / m, this->y / m, this->z / m);
}

template <class T>
Vector3f<T> Vector3f<T>::operator/(const Vector3f<T> &v) const
{
	return Vector3f<T>(this->x / v.x, this->y / v.y, this->z / v.z);
}

template <class T>
std::string Vector3f<T>::toString() const
{
	return std::to_string(this->x) + ", " + std::to_string(this->y) + ", " +
		std::to_string(this->z);
}

template <class T>
uint32_t Vector3f<T>::toRGB() const
{
	return static_cast<uint32_t>(
		((static_cast<uint8_t>(this->x * 255.0)) << 16) |
		((static_cast<uint8_t>(this->y * 255.0)) << 8) |
		((static_cast<uint8_t>(this->z * 255.0))));
}

template <class T>
T Vector3f<T>::lengthSquared() const
{
	return (this->x * this->x) + (this->y * this->y) + (this->z * this->z);
}

template <class T>
T Vector3f<T>::length() const
{
	return std::sqrt(this->lengthSquared());
}

template <class T>
Vector3f<T> Vector3f<T>::normalized() const
{
	T lenRecip = static_cast<T>(1.0) / this->length();
	return Vector3f<T>(this->x * lenRecip, this->y * lenRecip, this->z * lenRecip);
}

template <class T>
bool Vector3f<T>::isNormalized() const
{
	return std::fabs(static_cast<T>(1.0) - this->length()) <
		static_cast<T>(Constants::Epsilon);
}

template <class T>
T Vector3f<T>::dot(const Vector3f<T> &v) const
{
	return (this->x * v.x) + (this->y * v.y) + (this->z * v.z);
}

template <class T>
Vector3f<T> Vector3f<T>::cross(const Vector3f<T> &v) const
{
	return Vector3f<T>(
		(this->y * v.z) - (v.y * this->z),
		(v.x * this->z) - (this->x * v.z),
		(this->x * v.y) - (v.x * this->y));
}

template <class T>
Vector3f<T> Vector3f<T>::reflect(const Vector3f<T> &normal) const
{
	T vnDot = this->dot(normal);
	T vnSign = static_cast<T>((vnDot > 0.0) ? 1.0 : ((vnDot < 0.0) ? -1.0 : 0.0));
	T vnDot2 = static_cast<T>(vnDot * 2.0);
	return ((normal * vnSign) * vnDot2) - (*this);
}

template <class T>
Vector3f<T> Vector3f<T>::lerp(const Vector3f<T> &end, T percent) const
{
	return Vector3f<T>(
		this->x + ((end.x - this->x) * percent),
		this->y + ((end.y - this->y) * percent),
		this->z + ((end.z - this->z) * percent));
}

template <class T>
Vector3f<T> Vector3f<T>::slerp(const Vector3f<T> &end, T percent) const
{
	T theta = static_cast<T>(std::acos(this->dot(end) / (this->length() * end.length())));
	T sinThetaRecip = static_cast<T>(1.0 / std::sin(theta));
	T beginScale = static_cast<T>(std::sin((1.0 - percent) * theta) * sinThetaRecip);
	T endScale = static_cast<T>(std::sin(percent * theta) * sinThetaRecip);
	return ((*this) * beginScale) + (end * endScale);
}

template <class T>
Vector3f<T> Vector3f<T>::clamped(T low, T high) const
{
	return Vector3f<T>(
		(this->x > high) ? high : ((this->x < low) ? low : this->x),
		(this->y > high) ? high : ((this->y < low) ? low : this->y),
		(this->z > high) ? high : ((this->z < low) ? low : this->z));
}

template <class T>
Vector3f<T> Vector3f<T>::clamped() const
{
	const T low = static_cast<T>(0.0);
	const T high = static_cast<T>(1.0);
	return this->clamped(low, high);
}

template <class T>
Vector3f<T> Vector3f<T>::componentMin(const Vector3f<T> &v) const
{
	return Vector3f<T>(
		(this->x < v.x) ? this->x : v.x,
		(this->y < v.y) ? this->y : v.y,
		(this->z < v.z) ? this->z : v.z);
}

template <class T>
Vector3f<T> Vector3f<T>::componentMax(const Vector3f<T> &v) const
{
	return Vector3f<T>(
		(this->x > v.x) ? this->x : v.x,
		(this->y > v.y) ? this->y : v.y,
		(this->z > v.z) ? this->z : v.z);
}

// Template instantiations.
template class Vector3i<char>;
template class Vector3i<unsigned char>;
template class Vector3i<short>;
template class Vector3i<unsigned short>;
template class Vector3i<int>;
template class Vector3i<unsigned int>;

template class Vector3f<float>;
template class Vector3f<double>;
