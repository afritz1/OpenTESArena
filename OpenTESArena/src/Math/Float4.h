#ifndef FLOAT4_H
#define FLOAT4_H

#include <cstdint>
#include <string>
#include <type_traits>

class Color;

template <typename T>
class Float3;

template <typename T>
class Float4
{
	static_assert(std::is_floating_point<T>::value, "Float4<T> must be floating point.");
private:
	T x, y, z, w;
public:
	Float4(T x, T y, T z, T w);
	Float4(const Float3<T> &v, T w);
	Float4(const Float3<T> &v);
	Float4();
	~Float4();

	static Float4 fromARGB(uint32_t argb);
	static Float4 fromColor(const Color &c);

	Float4 operator +(const Float4 &v) const;
	Float4 operator -(const Float4 &v) const;
	Float4 operator -() const;

	T getX() const;
	T getY() const;
	T getZ() const;
	T getW() const;
	Float3<T> getXYZ() const;

	std::string toString() const;
	uint32_t toARGB() const;
	Color toColor() const;
};

// The template instantiations are at the end of the .cpp file.
typedef Float4<float> Float4f;
typedef Float4<double> Float4d;

#endif
