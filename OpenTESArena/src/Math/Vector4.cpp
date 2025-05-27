#include <cmath>
#include <cstdio>

#include "Vector4.h"

// -- Vector4i --

template<typename T>
T &Vector4i<T>::operator[](size_t index)
{
	return reinterpret_cast<T*>(&this->x)[index];
}

template<typename T>
const T &Vector4i<T>::operator[](size_t index) const
{
	return reinterpret_cast<const T*>(&this->x)[index];
}

template<typename T>
bool Vector4i<T>::operator==(const Vector4i<T> &v) const
{
	return (this->x == v.x) && (this->y == v.y) && (this->z == v.z) && (this->w == v.w);
}

template<typename T>
bool Vector4i<T>::operator!=(const Vector4i<T> &v) const
{
	return (this->x != v.x) || (this->y != v.y) || (this->z != v.z) || (this->w != v.w);
}

template<typename T>
Vector4i<T> Vector4i<T>::operator+(const Vector4i<T> &v) const
{
	return Vector4i<T>(this->x + v.x, this->y + v.y, this->z + v.z, this->w + v.w);
}

// operator-() is in the header.

template<typename T>
Vector4i<T> Vector4i<T>::operator-(const Vector4i<T> &v) const
{
	return Vector4i<T>(this->x - v.x, this->y - v.y, this->z - v.z, this->w - v.w);
}

template<typename T>
Vector4i<T> Vector4i<T>::operator*(T m) const
{
	return Vector4i<T>(this->x * m, this->y * m, this->z * m, this->w * m);
}

template<typename T>
Vector4i<T> Vector4i<T>::operator*(const Vector4i<T> &v) const
{
	return Vector4i<T>(this->x * v.x, this->y * v.y, this->z * v.z, this->w * v.w);
}

template<typename T>
Vector4i<T> Vector4i<T>::operator/(T m) const
{
	return Vector4i<T>(this->x / m, this->y / m, this->z / m, this->w / m);
}

template<typename T>
Vector4i<T> Vector4i<T>::operator/(const Vector4i<T> &v) const
{
	return Vector4i<T>(this->x / v.x, this->y / v.y, this->z / v.z, this->w / v.w);
}

template<typename T>
std::string Vector4i<T>::toString() const
{
	char buffer[128];
	std::snprintf(buffer, std::size(buffer), "%d, %d, %d, %d", this->x, this->y, this->z, this->w);
	return std::string(buffer);
}

// -- Vector4f --

template<typename T>
Vector4f<T> Vector4f<T>::fromARGB(uint32_t argb)
{
	const uint8_t r = static_cast<uint8_t>(argb >> 16);
	const uint8_t g = static_cast<uint8_t>(argb >> 8);
	const uint8_t b = static_cast<uint8_t>(argb);
	const uint8_t a = static_cast<uint8_t>(argb >> 24);
	return Vector4f<T>(
		static_cast<T>(r / 255.0),
		static_cast<T>(g / 255.0),
		static_cast<T>(b / 255.0),
		static_cast<T>(a / 255.0));
}

template<typename T>
Vector4f<T> Vector4f<T>::fromRGBA(uint32_t rgba)
{
	const uint8_t r = static_cast<uint8_t>(rgba >> 24);
	const uint8_t g = static_cast<uint8_t>(rgba >> 16);
	const uint8_t b = static_cast<uint8_t>(rgba >> 8);
	const uint8_t a = static_cast<uint8_t>(rgba);
	return Vector4f<T>(
		static_cast<T>(r / 255.0),
		static_cast<T>(g / 255.0),
		static_cast<T>(b / 255.0),
		static_cast<T>(a / 255.0));
}

template<typename T>
T &Vector4f<T>::operator[](size_t index)
{
	return reinterpret_cast<T*>(&this->x)[index];
}

template<typename T>
const T &Vector4f<T>::operator[](size_t index) const
{
	return reinterpret_cast<const T*>(&this->x)[index];
}

template<typename T>
bool Vector4f<T>::operator==(const Vector4f<T> &v) const
{
	return (this->x == v.x) && (this->y == v.y) && (this->z == v.z) && (this->w == v.w);
}

template<typename T>
bool Vector4f<T>::operator!=(const Vector4f<T> &v) const
{
	return (this->x != v.x) || (this->y != v.y) || (this->z != v.z) || (this->w != v.w);
}

template<typename T>
Vector4f<T> Vector4f<T>::operator+(const Vector4f<T> &v) const
{
	return Vector4f<T>(this->x + v.x, this->y + v.y, this->z + v.z, this->w + v.w);
}

template<typename T>
Vector4f<T> Vector4f<T>::operator-() const
{
	return Vector4f<T>(-this->x, -this->y, -this->z, -this->w);
}

template<typename T>
Vector4f<T> Vector4f<T>::operator-(const Vector4f<T> &v) const
{
	return Vector4f<T>(this->x - v.x, this->y - v.y, this->z - v.z, this->w - v.w);
}

template<typename T>
Vector4f<T> Vector4f<T>::operator*(T m) const
{
	return Vector4f<T>(this->x * m, this->y * m, this->z * m, this->w * m);
}

template<typename T>
Vector4f<T> Vector4f<T>::operator*(const Vector4f<T> &v) const
{
	return Vector4f<T>(this->x * v.x, this->y * v.y, this->z * v.z, this->w * v.w);
}

template<typename T>
Vector4f<T> Vector4f<T>::operator/(T m) const
{
	return Vector4f<T>(this->x / m, this->y / m, this->z / m, this->w / m);
}

template<typename T>
Vector4f<T> Vector4f<T>::operator/(const Vector4f<T> &v) const
{
	return Vector4f<T>(this->x / v.x, this->y / v.y, this->z / v.z, this->w / v.w);
}

template<typename T>
std::string Vector4f<T>::toString() const
{
	char buffer[128];
	std::snprintf(buffer, std::size(buffer), "%.2f, %.2f, %.2f, %.2f", this->x, this->y, this->z, this->w);
	return std::string(buffer);
}

template<typename T>
uint32_t Vector4f<T>::toARGB() const
{
	const uint8_t r = static_cast<uint8_t>(this->x * 255.0);
	const uint8_t g = static_cast<uint8_t>(this->y * 255.0);
	const uint8_t b = static_cast<uint8_t>(this->z * 255.0);
	const uint8_t a = static_cast<uint8_t>(this->w * 255.0);
	return static_cast<uint32_t>((r << 16) | (g << 8) | b | (a << 24));
}

template<typename T>
uint32_t Vector4f<T>::toRGBA() const
{
	const uint8_t r = static_cast<uint8_t>(this->x * 255.0);
	const uint8_t g = static_cast<uint8_t>(this->y * 255.0);
	const uint8_t b = static_cast<uint8_t>(this->z * 255.0);
	const uint8_t a = static_cast<uint8_t>(this->w * 255.0);
	return static_cast<uint32_t>((r << 24) | (g << 16) | (b << 8) | a);
}

template<typename T>
T Vector4f<T>::lengthSquared() const
{
	return (this->x * this->x) + (this->y * this->y) + (this->z * this->z) + (this->w * this->w);
}

template<typename T>
T Vector4f<T>::length() const
{
	return std::sqrt(this->lengthSquared());
}

template<typename T>
Vector4f<T> Vector4f<T>::lerp(const Vector4f<T> &end, T percent) const
{
	return Vector4f<T>(
		this->x + ((end.x - this->x) * percent),
		this->y + ((end.y - this->y) * percent),
		this->z + ((end.z - this->z) * percent),
		this->w + ((end.w - this->w) * percent));
}

template<typename T>
Vector4f<T> Vector4f<T>::clamped(T low, T high) const
{
	return Vector4f<T>(
		(this->x > high) ? high : ((this->x < low) ? low : this->x),
		(this->y > high) ? high : ((this->y < low) ? low : this->y),
		(this->z > high) ? high : ((this->z < low) ? low : this->z),
		(this->w > high) ? high : ((this->w < low) ? low : this->w));
}

template<typename T>
Vector4f<T> Vector4f<T>::clamped() const
{
	constexpr T low = static_cast<T>(0.0);
	constexpr T high = static_cast<T>(1.0);
	return this->clamped(low, high);
}

// Template instantiations.
template struct Vector4i<char>;
template struct Vector4i<unsigned char>;
template struct Vector4i<short>;
template struct Vector4i<unsigned short>;
template struct Vector4i<int>;
template struct Vector4i<unsigned int>;

template struct Vector4f<float>;
template struct Vector4f<double>;
