#include <cmath>
#include <cstdio>

#include "Constants.h"
#include "MathUtils.h"
#include "Random.h"
#include "Vector3.h"
#include "../Utilities/Endian.h"

// -- Vector3i --

template<typename T>
T &Vector3i<T>::operator[](size_t index)
{
	return reinterpret_cast<T*>(&this->x)[index];
}

template<typename T>
const T &Vector3i<T>::operator[](size_t index) const
{
	return reinterpret_cast<const T*>(&this->x)[index];
}

template<typename T>
bool Vector3i<T>::operator==(const Vector3i<T> &v) const
{
	return (this->x == v.x) && (this->y == v.y) && (this->z == v.z);
}

template<typename T>
bool Vector3i<T>::operator!=(const Vector3i<T> &v) const
{
	return (this->x != v.x) || (this->y != v.y) || (this->z != v.z);
}

template<typename T>
Vector3i<T> Vector3i<T>::operator+(const Vector3i<T> &v) const
{
	return Vector3i<T>(this->x + v.x, this->y + v.y, this->z + v.z);
}

// operator-() is in the header.

template<typename T>
Vector3i<T> Vector3i<T>::operator-(const Vector3i<T> &v) const
{
	return Vector3i<T>(this->x - v.x, this->y - v.y, this->z - v.z);
}

template<typename T>
Vector3i<T> Vector3i<T>::operator*(T m) const
{
	return Vector3i<T>(this->x * m, this->y * m, this->z * m);
}

template<typename T>
Vector3i<T> Vector3i<T>::operator*(const Vector3i<T> &v) const
{
	return Vector3i<T>(this->x * v.x, this->y * v.y, this->z * v.z);
}

template<typename T>
Vector3i<T> Vector3i<T>::operator/(T m) const
{
	return Vector3i<T>(this->x / m, this->y / m, this->z / m);
}

template<typename T>
Vector3i<T> Vector3i<T>::operator/(const Vector3i<T> &v) const
{
	return Vector3i<T>(this->x / v.x, this->y / v.y, this->z / v.z);
}

template<typename T>
Vector2i<T> Vector3i<T>::getXY() const
{
	return Vector2i<T>(this->x, this->y);
}

template<typename T>
Vector2i<T> Vector3i<T>::getXZ() const
{
	return Vector2i<T>(this->x, this->z);
}

template<typename T>
Vector2i<T> Vector3i<T>::getYZ() const
{
	return Vector2i<T>(this->y, this->z);
}

template<typename T>
std::string Vector3i<T>::toString() const
{
	char buffer[64];
	std::snprintf(buffer, std::size(buffer), "%d, %d, %d", this->x, this->y, this->z);
	return std::string(buffer);
}

template<typename T>
size_t Vector3i<T>::toHash() const
{
	size_t hash = 0;
	hash = MathUtils::hashCombine(hash, this->x);
	hash = MathUtils::hashCombine(hash, this->y);
	hash = MathUtils::hashCombine(hash, this->z);
	return hash;
}

// -- Vector3f --

template<typename T>
Vector3f<T> Vector3f<T>::randomDirection(Random &random)
{
	const T x = static_cast<T>((2.0 * random.nextReal()) - 1.0);
	const T y = static_cast<T>((2.0 * random.nextReal()) - 1.0);
	const T z = static_cast<T>((2.0 * random.nextReal()) - 1.0);
	return Vector3f<T>(x, y, z).normalized();
}

template<typename T>
Vector3f<T> Vector3f<T>::randomPointInSphere(const Vector3f<T> &center, T radius, Random &random)
{
	const Vector3f<T> randPoint = Vector3f<T>::randomDirection(random) * (radius * static_cast<T>(random.nextReal()));
	return Vector3f<T>(center.x + randPoint.x, center.y + randPoint.y, center.z + randPoint.z);
}

template<typename T>
Vector3f<T> Vector3f<T>::randomPointInCuboid(const Vector3f<T> &center, T width, T height, T depth, Random &random)
{
	return Vector3f<T>(
		center.x + (width * static_cast<T>(random.nextReal() - 0.50)),
		center.y + (height * static_cast<T>(random.nextReal() - 0.50)),
		center.z + (depth * static_cast<T>(random.nextReal() - 0.50)));
}

template<typename T>
Vector3f<T> Vector3f<T>::fromRGBx(uint32_t rgbx)
{
	const uint8_t r = static_cast<uint8_t>(rgbx >> Endian::RGBA_RedShift);
	const uint8_t g = static_cast<uint8_t>(rgbx >> Endian::RGBA_GreenShift);
	const uint8_t b = static_cast<uint8_t>(rgbx >> Endian::RGBA_BlueShift);
	return Vector3f<T>(
		static_cast<T>(r) / 255.0,
		static_cast<T>(g) / 255.0,
		static_cast<T>(b) / 255.0);
}

template<typename T>
T &Vector3f<T>::operator[](size_t index)
{
	return reinterpret_cast<T*>(&this->x)[index];
}

template<typename T>
const T &Vector3f<T>::operator[](size_t index) const
{
	return reinterpret_cast<const T*>(&this->x)[index];
}

template<typename T>
bool Vector3f<T>::operator==(const Vector3f<T> &v) const
{
	return (this->x == v.x) && (this->y == v.y) && (this->z == v.z);
}

template<typename T>
bool Vector3f<T>::operator!=(const Vector3f<T> &v) const
{
	return (this->x != v.x) || (this->y != v.y) || (this->z != v.z);
}

template<typename T>
Vector3f<T> Vector3f<T>::operator+(const Vector3f<T> &v) const
{
	return Vector3f<T>(this->x + v.x, this->y + v.y, this->z + v.z);
}

template<typename T>
Vector3f<T> Vector3f<T>::operator-() const
{
	return Vector3f<T>(-this->x, -this->y, -this->z);
}

template<typename T>
Vector3f<T> Vector3f<T>::operator-(const Vector3f<T> &v) const
{
	return Vector3f<T>(this->x - v.x, this->y - v.y, this->z - v.z);
}

template<typename T>
Vector3f<T> Vector3f<T>::operator*(T m) const
{
	return Vector3f<T>(this->x * m, this->y * m, this->z * m);
}

template<typename T>
Vector3f<T> Vector3f<T>::operator*(const Vector3f<T> &v) const
{
	return Vector3f<T>(this->x * v.x, this->y * v.y, this->z * v.z);
}

template<typename T>
Vector3f<T> Vector3f<T>::operator/(T m) const
{
	return Vector3f<T>(this->x / m, this->y / m, this->z / m);
}

template<typename T>
Vector3f<T> Vector3f<T>::operator/(const Vector3f<T> &v) const
{
	return Vector3f<T>(this->x / v.x, this->y / v.y, this->z / v.z);
}

template<typename T>
Vector2f<T> Vector3f<T>::getXY() const
{
	return Vector2f<T>(this->x, this->y);
}

template<typename T>
Vector2f<T> Vector3f<T>::getXZ() const
{
	return Vector2f<T>(this->x, this->z);
}

template<typename T>
Vector2f<T> Vector3f<T>::getYZ() const
{
	return Vector2f<T>(this->y, this->z);
}

template<typename T>
std::string Vector3f<T>::toString() const
{
	char buffer[64];
	std::snprintf(buffer, std::size(buffer), "%.2f, %.2f, %.2f", this->x, this->y, this->z);
	return std::string(buffer);
}

template<typename T>
uint32_t Vector3f<T>::toRGBA() const
{
	const uint8_t r = static_cast<uint8_t>(this->x * 255.0);
	const uint8_t g = static_cast<uint8_t>(this->y * 255.0);
	const uint8_t b = static_cast<uint8_t>(this->z * 255.0);
	constexpr uint8_t a = 255;
	return static_cast<uint32_t>(
		(r << Endian::RGBA_RedShift) |
		(g << Endian::RGBA_GreenShift) |
		(b << Endian::RGBA_BlueShift) |
		(a << Endian::RGBA_AlphaShift));
}

template<typename T>
double Vector3f<T>::getYAngleRadians() const
{
	// Get the length of the direction vector's projection onto the XZ plane.
	const double xzProjection = std::sqrt((this->x * this->x) + (this->z * this->z));

	if (this->y > 0.0)
	{
		// Above the horizon.
		return std::acos(xzProjection);
	}
	else if (this->y < 0.0)
	{
		// Below the horizon.
		return -std::acos(xzProjection);
	}
	else
	{
		// At the horizon.
		return 0.0;
	}
}

template<typename T>
T Vector3f<T>::lengthSquared() const
{
	return (this->x * this->x) + (this->y * this->y) + (this->z * this->z);
}

template<typename T>
T Vector3f<T>::length() const
{
	return std::sqrt(this->lengthSquared());
}

template<typename T>
Vector3f<T> Vector3f<T>::normalized() const
{
	const T lenRecip = static_cast<T>(1.0) / this->length();
	return Vector3f<T>(this->x * lenRecip, this->y * lenRecip, this->z * lenRecip);
}

template<typename T>
bool Vector3f<T>::isNormalized() const
{
	return std::fabs(static_cast<T>(1.0) - this->length()) < static_cast<T>(Constants::Epsilon);
}

template<typename T>
T Vector3f<T>::dot(const Vector3f<T> &v) const
{
	return (this->x * v.x) + (this->y * v.y) + (this->z * v.z);
}

template<typename T>
Vector3f<T> Vector3f<T>::cross(const Vector3f<T> &v) const
{
	return Vector3f<T>(
		(this->y * v.z) - (v.y * this->z),
		(v.x * this->z) - (this->x * v.z),
		(this->x * v.y) - (v.x * this->y));
}

template<typename T>
Vector3f<T> Vector3f<T>::reflect(const Vector3f<T> &normal) const
{
	const T vnDot = this->dot(normal);
	const T vnSign = static_cast<T>((vnDot > 0.0) ? 1.0 : ((vnDot < 0.0) ? -1.0 : 0.0));
	const T vnDot2 = static_cast<T>(vnDot * 2.0);
	return ((normal * vnSign) * vnDot2) - (*this);
}

template<typename T>
Vector3f<T> Vector3f<T>::lerp(const Vector3f<T> &end, T percent) const
{
	return Vector3f<T>(
		this->x + ((end.x - this->x) * percent),
		this->y + ((end.y - this->y) * percent),
		this->z + ((end.z - this->z) * percent));
}

template<typename T>
Vector3f<T> Vector3f<T>::slerp(const Vector3f<T> &end, T percent) const
{
	const T theta = static_cast<T>(std::acos(this->dot(end) / (this->length() * end.length())));
	const T sinThetaRecip = static_cast<T>(1.0 / std::sin(theta));
	const T beginScale = static_cast<T>(std::sin((1.0 - percent) * theta) * sinThetaRecip);
	const T endScale = static_cast<T>(std::sin(percent * theta) * sinThetaRecip);
	return ((*this) * beginScale) + (end * endScale);
}

template<typename T>
Vector3f<T> Vector3f<T>::clamped(T low, T high) const
{
	return Vector3f<T>(
		(this->x > high) ? high : ((this->x < low) ? low : this->x),
		(this->y > high) ? high : ((this->y < low) ? low : this->y),
		(this->z > high) ? high : ((this->z < low) ? low : this->z));
}

template<typename T>
Vector3f<T> Vector3f<T>::clamped() const
{
	const T low = static_cast<T>(0.0);
	const T high = static_cast<T>(1.0);
	return this->clamped(low, high);
}

template<typename T>
Vector3f<T> Vector3f<T>::componentMin(const Vector3f<T> &v) const
{
	return Vector3f<T>(
		(this->x < v.x) ? this->x : v.x,
		(this->y < v.y) ? this->y : v.y,
		(this->z < v.z) ? this->z : v.z);
}

template<typename T>
Vector3f<T> Vector3f<T>::componentMax(const Vector3f<T> &v) const
{
	return Vector3f<T>(
		(this->x > v.x) ? this->x : v.x,
		(this->y > v.y) ? this->y : v.y,
		(this->z > v.z) ? this->z : v.z);
}

// Template instantiations.
template struct Vector3i<char>;
template struct Vector3i<unsigned char>;
template struct Vector3i<short>;
template struct Vector3i<unsigned short>;
template struct Vector3i<int>;
template struct Vector3i<unsigned int>;

template struct Vector3f<float>;
template struct Vector3f<double>;
