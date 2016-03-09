#include "Vector2.h"
#include "Constants.h"
#include "Random.h"

template<typename T>
Random Vector2<T>::random = Random();

template<typename T>
Vector2<T>::Vector2(T x, T y)
{
	this->x = x;
	this->y = y;
}

template<typename T>
Vector2<T>::Vector2()
	: Vector2<T>(T(), T()) { }

template<typename T>
Vector2<T>::~Vector2()
{

}

template<typename T>
Vector2<T> Vector2<T>::randomDirection()
{
	T x = static_cast<T>((2.0 * Vector2::random.nextReal()) - 1.0);
	T y = static_cast<T>((2.0 * Vector2::random.nextReal()) - 1.0);
	return Vector2(x, y).normalized();
}

template<typename T>
Vector2<T> Vector2<T>::randomPointInCircle(const Vector2 &center, T radius)
{
	T scale = radius * static_cast<T>(Vector2::random.nextReal());
	auto randPoint = Vector2::randomDirection().scaledBy(scale);
	return Vector2(center.x + randPoint.x, center.y + randPoint.y);
}

template<typename T>
Vector2<T> Vector2<T>::randomPointInSquare(const Vector2 &center, T width, T height)
{
	auto randDirection = Vector2::randomDirection();
	return Vector2(
		center.x + (width * randDirection.x),
		center.y + (height * randDirection.y));
}

template<typename T>
Vector2<T> Vector2<T>::operator +(const Vector2 &v) const
{
	return Vector2(this->x + v.x, this->y + v.y);
}

template<typename T>
Vector2<T> Vector2<T>::operator -(const Vector2 &v) const
{
	return Vector2(this->x - v.x, this->y - v.y);
}

template<typename T>
Vector2<T> Vector2<T>::operator -() const
{
	return Vector2(-this->x, -this->y);
}

template<typename T>
const T &Vector2<T>::getX() const
{
	return this->x;
}

template<typename T>
const T &Vector2<T>::getY() const
{
	return this->y;
}

template<typename T>
std::string Vector2<T>::toString() const
{
	return std::string("[") +
		std::to_string(this->x) + std::string(", ") +
		std::to_string(this->y) + std::string("]");
}

template<typename T>
T Vector2<T>::lengthSquared() const
{
	return (this->x * this->x) + (this->y * this->y);
}

template<typename T>
T Vector2<T>::length() const
{
	return std::sqrt(this->lengthSquared());
}

template<typename T>
Vector2<T> Vector2<T>::normalized() const
{
	T lenRecip = static_cast<T>(1.0) / this->length();
	return Vector2(this->x * lenRecip, this->y * lenRecip);
}

template<typename T>
bool Vector2<T>::isNormalized() const
{
	return std::fabs(static_cast<T>(1.0) - this->length()) < EPSILON;
}

template<typename T>
T Vector2<T>::dot(T m) const
{
	return (this->x * m) + (this->y * m);
}

template<typename T>
T Vector2<T>::dot(const Vector2 &v) const
{
	return (this->x * v.x) + (this->y * v.y);
}

template<typename T>
Vector2<T> Vector2<T>::scaledBy(T m) const
{
	return Vector2(this->x * m, this->y * m);
}

template<typename T>
Vector2<T> Vector2<T>::scaledBy(const Vector2 &v) const
{
	return Vector2(this->x * v.x, this->y * v.y);
}

template<typename T>
Vector2<T> Vector2<T>::lerp(const Vector2 &end, T percent) const
{
	return Vector2(
		this->x + ((end.x - this->x) * percent),
		this->y + ((end.y - this->y) * percent));
}

template<typename T>
Vector2<T> Vector2<T>::slerp(const Vector2 &end, T percent) const
{
	T theta = static_cast<T>(std::acos(this->dot(end) / (this->length() * end.length())));
	T sinThetaRecip = static_cast<T>(1.0 / std::sin(theta));
	T beginScale = static_cast<T>(std::sin((1.0 - percent) * theta) * sinThetaRecip);
	T endScale = std::sin(percent * theta) * sinThetaRecip;
	return this->scaledBy(beginScale) + end.scaledBy(endScale);
}

template<typename T>
Vector2<T> Vector2<T>::leftPerp() const
{
	return Vector2(-this->y, this->x);
}

template<typename T>
Vector2<T> Vector2<T>::rightPerp() const
{
	return Vector2(this->y, this->x);
}

// Template instantiations.
template class Vector2<float>;
template class Vector2<double>;