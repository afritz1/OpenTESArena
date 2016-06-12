#include "Float4.h"

#include "Float3.h"
#include "../Media/Color.h"

template<typename T>
Float4<T>::Float4(T x, T y, T z, T w)
{
	this->x = x;
	this->y = y;
	this->z = z;
	this->w = w;
}

template<typename T>
Float4<T>::Float4(const Float3<T> &v, T w)
	: Float4(v.getX(), v.getY(), v.getZ(), w) { }

template<typename T>
Float4<T>::Float4(const Float3<T> &v)
	: Float4(v.getX(), v.getY(), v.getZ(), T()) { }

template<typename T>
Float4<T>::Float4()
	: Float4<T>(T(), T(), T(), T()) { }

template<typename T>
Float4<T>::~Float4()
{

}

template<typename T>
Float4<T> Float4<T>::fromARGB(unsigned int argb)
{
	return Float4(
		static_cast<T>((static_cast<unsigned char>(argb) << 16) / 255.0),
		static_cast<T>((static_cast<unsigned char>(argb) << 8) / 255.0),
		static_cast<T>((static_cast<unsigned char>(argb)) / 255.0),
		static_cast<T>((static_cast<unsigned char>(argb) << 24) / 255.0));
}

template<typename T>
Float4<T> Float4<T>::fromColor(const Color &c)
{
	return Float4(
		static_cast<T>(c.getR() / 255.0),
		static_cast<T>(c.getG() / 255.0),
		static_cast<T>(c.getB() / 255.0),
		static_cast<T>(c.getA() / 255.0));
}

template<typename T>
Float4<T> Float4<T>::operator +(const Float4 &v) const
{
	return Float4(this->x + v.x, this->y + v.y, this->z + v.z, this->w + v.w);
}

template<typename T>
Float4<T> Float4<T>::operator -(const Float4 &v) const
{
	return Float4(this->x - v.x, this->y - v.y, this->z - v.z, this->w - v.w);
}

template<typename T>
Float4<T> Float4<T>::operator -() const
{
	return Float4(-this->x, -this->y, -this->z, -this->w);
}

template<typename T>
T Float4<T>::getX() const
{
	return this->x;
}

template<typename T>
T Float4<T>::getY() const
{
	return this->y;
}

template<typename T>
T Float4<T>::getZ() const
{
	return this->z;
}

template<typename T>
T Float4<T>::getW() const
{
	return this->w;
}

template<typename T>
Float3<T> Float4<T>::getXYZ() const
{
	return Float3<T>(this->x, this->y, this->z);
}

template<typename T>
std::string Float4<T>::toString() const
{
	return std::string("[") +
		std::to_string(this->x) + std::string(", ") +
		std::to_string(this->y) + std::string(", ") +
		std::to_string(this->z) + std::string(", ") +
		std::to_string(this->w) + std::string("]");
}

template<typename T>
unsigned int Float4<T>::toARGB() const
{
	return static_cast<unsigned int>(
		((static_cast<unsigned char>(this->x * 255.0)) << 16) |
		((static_cast<unsigned char>(this->y * 255.0)) << 8) |
		((static_cast<unsigned char>(this->z * 255.0))) |
		((static_cast<unsigned char>(this->w * 255.0)) << 24));
}

template<typename T>
Color Float4<T>::toColor() const
{
	auto r = static_cast<unsigned char>(this->x * 255.0);
	auto g = static_cast<unsigned char>(this->y * 255.0);
	auto b = static_cast<unsigned char>(this->z * 255.0);
	auto a = static_cast<unsigned char>(this->w * 255.0);
	return Color(r, g, b, a);
}

// Template instantiations.
template class Float4<float>;
template class Float4<double>;
