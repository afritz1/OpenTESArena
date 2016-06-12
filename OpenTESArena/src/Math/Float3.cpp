#include "Float3.h"

#include "Constants.h"
#include "Random.h"
#include "../Media/Color.h"

template<typename T>
Float3<T>::Float3(T x, T y, T z)
{
	this->x = x;
	this->y = y;
	this->z = z;
}

template<typename T>
Float3<T>::Float3()
	: Float3<T>(T(), T(), T()) { }

template<typename T>
Float3<T>::~Float3()
{

}

template<typename T>
Float3<T> Float3<T>::randomDirection(Random &random)
{
	T x = static_cast<T>((2.0 * random.nextReal()) - 1.0);
	T y = static_cast<T>((2.0 * random.nextReal()) - 1.0);
	T z = static_cast<T>((2.0 * random.nextReal()) - 1.0);
	return Float3(x, y, z).normalized();
}

template<typename T>
Float3<T> Float3<T>::randomPointInSphere(const Float3 &center, T radius, Random &random)
{
	T scale = radius * static_cast<T>(random.nextReal());
	auto randPoint = Float3::randomDirection(random).scaledBy(scale);
	return Float3(
		center.x + randPoint.x,
		center.y + randPoint.y,
		center.z + randPoint.z);
}

template<typename T>
Float3<T> Float3<T>::randomPointInCuboid(const Float3 &center, T width, T height, T depth,
	Random &random)
{
	auto randPoint = Float3(
		static_cast<T>((2.0 * random.nextReal()) - 1.0),
		static_cast<T>((2.0 * random.nextReal()) - 1.0),
		static_cast<T>((2.0 * random.nextReal()) - 1.0));
	return Float3(
		center.x + (width * randPoint.x),
		center.y + (height * randPoint.y),
		center.z + (depth * randPoint.z));
}

template<typename T>
Float3<T> Float3<T>::fromRGB(unsigned int rgb)
{
	return Float3(
		static_cast<T>((static_cast<unsigned char>(rgb) << 16) / 255.0),
		static_cast<T>((static_cast<unsigned char>(rgb) << 8) / 255.0),
		static_cast<T>((static_cast<unsigned char>(rgb)) / 255.0));
}

template<typename T>
Float3<T> Float3<T>::fromColor(const Color &c)
{
	return Float3(
		static_cast<T>(c.getR() / 255.0),
		static_cast<T>(c.getG() / 255.0),
		static_cast<T>(c.getB() / 255.0));
}

template<typename T>
Float3<T> Float3<T>::operator +(const Float3 &v) const
{
	return Float3(this->x + v.x, this->y + v.y, this->z + v.z);
}

template<typename T>
Float3<T> Float3<T>::operator -(const Float3 &v) const
{
	return Float3(this->x - v.x, this->y - v.y, this->z - v.z);
}

template<typename T>
Float3<T> Float3<T>::operator -() const
{
	return Float3(-this->x, -this->y, -this->z);
}

template<typename T>
T Float3<T>::getX() const
{
	return this->x;
}

template<typename T>
T Float3<T>::getY() const
{
	return this->y;
}

template<typename T>
T Float3<T>::getZ() const
{
	return this->z;
}

template<typename T>
std::string Float3<T>::toString() const
{
	return std::string("[") +
		std::to_string(this->x) + std::string(", ") +
		std::to_string(this->y) + std::string(", ") +
		std::to_string(this->z) + std::string("]");
}

template<typename T>
unsigned int Float3<T>::toRGB() const
{
	return static_cast<unsigned int>(
		((static_cast<unsigned char>(this->x * 255.0)) << 16) |
		((static_cast<unsigned char>(this->y * 255.0)) << 8) |
		((static_cast<unsigned char>(this->z * 255.0))));
}

template<typename T>
Color Float3<T>::toColor() const
{
	auto r = static_cast<unsigned char>(this->x * 255.0);
	auto g = static_cast<unsigned char>(this->y * 255.0);
	auto b = static_cast<unsigned char>(this->z * 255.0);
	return Color(r, g, b);
}

template<typename T>
T Float3<T>::lengthSquared() const
{
	return (this->x * this->x) + (this->y * this->y) + (this->z * this->z);
}

template<typename T>
T Float3<T>::length() const
{
	return std::sqrt(this->lengthSquared());
}

template<typename T>
Float3<T> Float3<T>::normalized() const
{
	T lenRecip = static_cast<T>(1.0) / this->length();
	return Float3(
		this->x * lenRecip,
		this->y * lenRecip,
		this->z * lenRecip);
}

template<typename T>
bool Float3<T>::isNormalized() const
{
	return std::fabs(static_cast<T>(1.0) - this->length()) < EPSILON;
}

template<typename T>
T Float3<T>::dot(T m) const
{
	return (this->x + this->y + this->z) * m;
}

template<typename T>
T Float3<T>::dot(const Float3 &v) const
{
	return (this->x * v.x) + (this->y * v.y) + (this->z * v.z);
}

template<typename T>
Float3<T> Float3<T>::cross(const Float3 &v) const
{
	return Float3<T>(
		(this->y * v.z) - (v.y * this->z),
		(v.x * this->z) - (this->x * v.z),
		(this->x * v.y) - (v.x * this->y));
}

template<typename T>
Float3<T> Float3<T>::reflect(const Float3 &normal) const
{
	T vnDot = this->dot(normal);
	T vnSign = static_cast<T>((vnDot > 0.0) ? 1.0 : ((vnDot < 0.0) ? -1.0 : 0.0));
	T vnDot2 = static_cast<T>(vnDot * 2.0);
	return Float3<T>(
		((vnSign * normal.x) * vnDot2) - this->x,
		((vnSign * normal.y) * vnDot2) - this->y,
		((vnSign * normal.z) * vnDot2) - this->z);
}

template<typename T>
Float3<T> Float3<T>::scaledBy(T m) const
{
	return Float3(this->x * m, this->y * m, this->z * m);
}

template<typename T>
Float3<T> Float3<T>::scaledBy(const Float3 &v) const
{
	return Float3(this->x * v.x, this->y * v.y, this->z * v.z);
}

template<typename T>
Float3<T> Float3<T>::lerp(const Float3 &end, T percent) const
{
	return Float3<T>(
		this->x + ((end.x - this->x) * percent),
		this->y + ((end.y - this->y) * percent),
		this->z + ((end.z - this->z) * percent));
}

template<typename T>
Float3<T> Float3<T>::slerp(const Float3 &end, T percent) const
{
	T theta = static_cast<T>(std::acos(this->dot(end) / (this->length() * end.length())));
	T sinThetaRecip = static_cast<T>(1.0 / std::sin(theta));
	T beginScale = static_cast<T>(std::sin((1.0 - percent) * theta) * sinThetaRecip);
	T endScale = static_cast<T>(std::sin(percent * theta) * sinThetaRecip);
	return this->scaledBy(beginScale) + end.scaledBy(endScale);
}

template<typename T>
Float3<T> Float3<T>::clamped(T low, T high) const
{
	return Float3<T>(
		(this->x > high) ? high : ((this->x < low) ? low : this->x),
		(this->y > high) ? high : ((this->y < low) ? low : this->y),
		(this->z > high) ? high : ((this->z < low) ? low : this->z));
}

template<typename T>
Float3<T> Float3<T>::clamped() const
{
	const T low = static_cast<T>(0.0);
	const T high = static_cast<T>(1.0);
	return this->clamped(low, high);
}

template<typename T>
Float3<T> Float3<T>::componentMin(const Float3 &v) const
{
	return Float3<T>(
		(this->x < v.x) ? this->x : v.x,
		(this->y < v.y) ? this->y : v.y,
		(this->z < v.z) ? this->z : v.z);
}

template<typename T>
Float3<T> Float3<T>::componentMax(const Float3 &v) const
{
	return Float3<T>(
		(this->x > v.x) ? this->x : v.x,
		(this->y > v.y) ? this->y : v.y,
		(this->z > v.z) ? this->z : v.z);
}

// Template instantiations.
template class Float3<float>;
template class Float3<double>;
