#include <cmath>

#include "Vector4.h"

// -- Vector4i --

template <class T>
Vector4i<T>::Vector4i(T x, T y, T z, T w)
{
	this->x = x;
	this->y = y;
	this->z = z;
	this->w = w;
}

template <class T>
Vector4i<T>::Vector4i()
{
	this->x = static_cast<T>(0);
	this->y = static_cast<T>(0);
	this->z = static_cast<T>(0);
	this->w = static_cast<T>(0);
}

template <class T>
T &Vector4i<T>::operator[](size_t index)
{
	return reinterpret_cast<T*>(&this->x)[index];
}

template <class T>
const T &Vector4i<T>::operator[](size_t index) const
{
	return reinterpret_cast<const T*>(&this->x)[index];
}

template <class T>
bool Vector4i<T>::operator==(const Vector4i<T> &v) const
{
	return (this->x == v.x) && (this->y == v.y) && (this->z == v.z) &&
		(this->w == v.w);
}

template <class T>
bool Vector4i<T>::operator!=(const Vector4i<T> &v) const
{
	return (this->x != v.x) || (this->y != v.y) || (this->z != v.z) ||
		(this->w != v.w);
}

template <class T>
Vector4i<T> Vector4i<T>::operator+(const Vector4i<T> &v) const
{
	return Vector4i<T>(this->x + v.x, this->y + v.y, this->z + v.z,
		this->w + v.w);
}

// operator-() is in the header.

template <class T>
Vector4i<T> Vector4i<T>::operator-(const Vector4i<T> &v) const
{
	return Vector4i<T>(this->x - v.x, this->y - v.y, this->z - v.z,
		this->w - v.w);
}

template <class T>
Vector4i<T> Vector4i<T>::operator*(T m) const
{
	return Vector4i<T>(this->x * m, this->y * m, this->z * m, this->w * m);
}

template <class T>
Vector4i<T> Vector4i<T>::operator*(const Vector4i<T> &v) const
{
	return Vector4i<T>(this->x * v.x, this->y * v.y, this->z * v.z,
		this->w * v.w);
}

template <class T>
Vector4i<T> Vector4i<T>::operator/(T m) const
{
	return Vector4i<T>(this->x / m, this->y / m, this->z / m, this->w / m);
}

template <class T>
Vector4i<T> Vector4i<T>::operator/(const Vector4i<T> &v) const
{
	return Vector4i<T>(this->x / v.x, this->y / v.y, this->z / v.z,
		this->w / v.w);
}

template <class T>
std::string Vector4i<T>::toString() const
{
	return std::to_string(this->x) + ", " + std::to_string(this->y) + ", " +
		std::to_string(this->z) + ", " + std::to_string(this->w);
}

// -- Vector4f --

template <class T>
Vector4f<T>::Vector4f(T x, T y, T z, T w)
{
	this->x = x;
	this->y = y;
	this->z = z;
	this->w = w;
}

template <class T>
Vector4f<T>::Vector4f(const Vector3f<T> &xyz, T w)
{
	this->x = xyz.x;
	this->y = xyz.y;
	this->z = xyz.z;
	this->w = w;
}

template<>
Vector4f<double>::Vector4f(const Vector4f<float>& old)
{
	this->x = old.x;
	this->y = old.y;
	this->z = old.z;
	this->w = old.w;
}

template<>
Vector4f<float>::Vector4f(const Vector4f<double>& old)
{
	this->x = old.x;
	this->y = old.y;
	this->z = old.z;
	this->w = old.w;
}

template<>
Vector4f<float>::Vector4f(const Vector4f<float>& old)
{
	this->x = old.x;
	this->y = old.y;
	this->z = old.z;
	this->w = old.w;
}

template<>
Vector4f<double>::Vector4f(const Vector4f<double>& old)
{
	this->x = old.x;
	this->y = old.y;
	this->z = old.z;
	this->w = old.w;
}

template <class T>
Vector4f<T>::Vector4f()
{
	this->x = static_cast<T>(0.0);
	this->y = static_cast<T>(0.0);
	this->z = static_cast<T>(0.0);
	this->w = static_cast<T>(0.0);
}

template <class T>
Vector4f<T> Vector4f<T>::fromARGB(uint32_t argb)
{
	return Vector4f<T>(
		static_cast<T>(static_cast<uint8_t>(argb >> 16) / 255.0),
		static_cast<T>(static_cast<uint8_t>(argb >> 8) / 255.0),
		static_cast<T>(static_cast<uint8_t>(argb) / 255.0),
		static_cast<T>(static_cast<uint8_t>(argb >> 24) / 255.0));
}

template <class T>
Vector4f<T> Vector4f<T>::fromRGBA(uint32_t rgba)
{
	return Vector4f<T>(
		static_cast<T>(static_cast<uint8_t>(rgba >> 24) / 255.0),
		static_cast<T>(static_cast<uint8_t>(rgba >> 16) / 255.0),
		static_cast<T>(static_cast<uint8_t>(rgba >> 8) / 255.0),
		static_cast<T>(static_cast<uint8_t>(rgba) / 255.0));
}

template <class T>
T &Vector4f<T>::operator[](size_t index)
{
	return reinterpret_cast<T*>(&this->x)[index];
}

template <class T>
const T &Vector4f<T>::operator[](size_t index) const
{
	return reinterpret_cast<const T*>(&this->x)[index];
}

template <class T>
bool Vector4f<T>::operator==(const Vector4f<T> &v) const
{
	return (this->x == v.x) && (this->y == v.y) && (this->z == v.z) &&
		(this->w == v.w);
}

template <class T>
bool Vector4f<T>::operator!=(const Vector4f<T> &v) const
{
	return (this->x != v.x) || (this->y != v.y) || (this->z != v.z) ||
		(this->w != v.w);
}

template <class T>
Vector4f<T> Vector4f<T>::operator+(const Vector4f<T> &v) const
{
	return Vector4f<T>(this->x + v.x, this->y + v.y, this->z + v.z,
		this->w + v.w);
}

template <class T>
Vector4f<T> Vector4f<T>::operator-() const
{
	return Vector4f<T>(-this->x, -this->y, -this->z, -this->w);
}

template <class T>
Vector4f<T> Vector4f<T>::operator-(const Vector4f<T> &v) const
{
	return Vector4f<T>(this->x - v.x, this->y - v.y, this->z - v.z,
		this->w - v.w);
}

template <class T>
Vector4f<T> Vector4f<T>::operator*(T m) const
{
	return Vector4f<T>(this->x * m, this->y * m, this->z * m, this->w * m);
}

template <class T>
Vector4f<T> Vector4f<T>::operator*(const Vector4f<T> &v) const
{
	return Vector4f<T>(this->x * v.x, this->y * v.y, this->z * v.z,
		this->w * v.w);
}

template <class T>
Vector4f<T> Vector4f<T>::operator/(T m) const
{
	return Vector4f<T>(this->x / m, this->y / m, this->z / m, this->w / m);
}

template <class T>
Vector4f<T> Vector4f<T>::operator/(const Vector4f<T> &v) const
{
	return Vector4f<T>(this->x / v.x, this->y / v.y, this->z / v.z,
		this->w / v.w);
}

template <class T>
std::string Vector4f<T>::toString() const
{
	return std::to_string(this->x) + ", " + std::to_string(this->y) + ", " +
		std::to_string(this->z) + ", " + std::to_string(this->w);
}

template <class T>
uint32_t Vector4f<T>::toARGB() const
{
	return static_cast<uint32_t>(
		((static_cast<uint8_t>(this->x * 255.0)) << 16) |
		((static_cast<uint8_t>(this->y * 255.0)) << 8) |
		((static_cast<uint8_t>(this->z * 255.0))) |
		((static_cast<uint8_t>(this->w * 255.0)) << 24));
}

template <class T>
uint32_t Vector4f<T>::toRGBA() const
{
	return static_cast<uint32_t>(
		((static_cast<uint8_t>(this->x * 255.0)) << 24) |
		((static_cast<uint8_t>(this->y * 255.0)) << 16) |
		((static_cast<uint8_t>(this->z * 255.0)) << 8) |
		((static_cast<uint8_t>(this->w * 255.0))));
}

template <class T>
T Vector4f<T>::lengthSquared() const
{
	return (this->x * this->x) + (this->y * this->y) + (this->z * this->z) +
		(this->w * this->w);
}

template <class T>
T Vector4f<T>::length() const
{
	return std::sqrt(this->lengthSquared());
}

template <class T>
Vector4f<T> Vector4f<T>::lerp(const Vector4f<T> &end, T percent) const
{
	return Vector4f<T>(
		this->x + ((end.x - this->x) * percent),
		this->y + ((end.y - this->y) * percent),
		this->z + ((end.z - this->z) * percent),
		this->w + ((end.w - this->w) * percent));
}

template <class T>
Vector4f<T> Vector4f<T>::clamped(T low, T high) const
{
	return Vector4f<T>(
		(this->x > high) ? high : ((this->x < low) ? low : this->x),
		(this->y > high) ? high : ((this->y < low) ? low : this->y),
		(this->z > high) ? high : ((this->z < low) ? low : this->z),
		(this->w > high) ? high : ((this->w < low) ? low : this->w));
}

template <class T>
Vector4f<T> Vector4f<T>::clamped() const
{
	const T low = static_cast<T>(0.0);
	const T high = static_cast<T>(1.0);
	return this->clamped(low, high);
}

// Template instantiations.
template class Vector4i<char>;
template class Vector4i<unsigned char>;
template class Vector4i<short>;
template class Vector4i<unsigned short>;
template class Vector4i<int>;
template class Vector4i<unsigned int>;

template class Vector4f<float>;
template class Vector4f<double>;
