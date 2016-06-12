#include "Float2.h"

#include "Constants.h"
#include "Random.h"

template<typename T>
Float2<T>::Float2(T x, T y)
{
	this->x = x;
	this->y = y;
}

template<typename T>
Float2<T>::Float2()
	: Float2<T>(T(), T()) { }

template<typename T>
Float2<T>::~Float2()
{

}

template<typename T>
Float2<T> Float2<T>::randomDirection(Random &random)
{
	T x = static_cast<T>((2.0 * random.nextReal()) - 1.0);
	T y = static_cast<T>((2.0 * random.nextReal()) - 1.0);
	return Float2(x, y).normalized();
}

template<typename T>
Float2<T> Float2<T>::randomPointInCircle(const Float2 &center, T radius, Random &random)
{
	T scale = radius * static_cast<T>(random.nextReal());
	auto randPoint = Float2::randomDirection(random).scaledBy(scale);
	return Float2(center.x + randPoint.x, center.y + randPoint.y);
}

template<typename T>
Float2<T> Float2<T>::randomPointInSquare(const Float2 &center, T width, T height, 
	Random &random)
{
	auto randDirection = Float2::randomDirection(random);
	return Float2(
		center.x + (width * randDirection.x),
		center.y + (height * randDirection.y));
}

template<typename T>
Float2<T> Float2<T>::operator +(const Float2 &v) const
{
	return Float2(this->x + v.x, this->y + v.y);
}

template<typename T>
Float2<T> Float2<T>::operator -(const Float2 &v) const
{
	return Float2(this->x - v.x, this->y - v.y);
}

template<typename T>
Float2<T> Float2<T>::operator -() const
{
	return Float2(-this->x, -this->y);
}

template<typename T>
T Float2<T>::getX() const
{
	return this->x;
}

template<typename T>
T Float2<T>::getY() const
{
	return this->y;
}

template<typename T>
std::string Float2<T>::toString() const
{
	return std::string("[") +
		std::to_string(this->x) + std::string(", ") +
		std::to_string(this->y) + std::string("]");
}

template<typename T>
T Float2<T>::lengthSquared() const
{
	return (this->x * this->x) + (this->y * this->y);
}

template<typename T>
T Float2<T>::length() const
{
	return std::sqrt(this->lengthSquared());
}

template<typename T>
Float2<T> Float2<T>::normalized() const
{
	T lenRecip = static_cast<T>(1.0) / this->length();
	return Float2(this->x * lenRecip, this->y * lenRecip);
}

template<typename T>
bool Float2<T>::isNormalized() const
{
	return std::fabs(static_cast<T>(1.0) - this->length()) < EPSILON;
}

template<typename T>
T Float2<T>::dot(T m) const
{
	return (this->x * m) + (this->y * m);
}

template<typename T>
T Float2<T>::dot(const Float2 &v) const
{
	return (this->x * v.x) + (this->y * v.y);
}

template<typename T>
Float2<T> Float2<T>::scaledBy(T m) const
{
	return Float2(this->x * m, this->y * m);
}

template<typename T>
Float2<T> Float2<T>::scaledBy(const Float2 &v) const
{
	return Float2(this->x * v.x, this->y * v.y);
}

template<typename T>
Float2<T> Float2<T>::lerp(const Float2 &end, T percent) const
{
	return Float2(
		this->x + ((end.x - this->x) * percent),
		this->y + ((end.y - this->y) * percent));
}

template<typename T>
Float2<T> Float2<T>::slerp(const Float2 &end, T percent) const
{
	T theta = static_cast<T>(std::acos(this->dot(end) / (this->length() * end.length())));
	T sinThetaRecip = static_cast<T>(1.0 / std::sin(theta));
	T beginScale = static_cast<T>(std::sin((1.0 - percent) * theta) * sinThetaRecip);
	T endScale = std::sin(percent * theta) * sinThetaRecip;
	return this->scaledBy(beginScale) + end.scaledBy(endScale);
}

template<typename T>
Float2<T> Float2<T>::leftPerp() const
{
	return Float2(-this->y, this->x);
}

template<typename T>
Float2<T> Float2<T>::rightPerp() const
{
	return Float2(this->y, this->x);
}

// Template instantiations.
template class Float2<float>;
template class Float2<double>;
