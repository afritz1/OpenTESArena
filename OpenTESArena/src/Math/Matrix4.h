#ifndef MATRIX4_H
#define MATRIX4_H

#include <string>
#include <type_traits>

#include "Vector3.h"
#include "Vector4.h"

template <typename T>
class Matrix4
{
public:
	static_assert(std::is_floating_point<T>::value, "Matrix4<T> must be floating point.");

	// Column vectors.
	Vector4f<T> x;
	Vector4f<T> y;
	Vector4f<T> z;
	Vector4f<T> w;

	Matrix4(const Vector4f<T> &x, const Vector4f<T> &y, const Vector4f<T> &z,
		const Vector4f<T> &w);
	Matrix4();
	~Matrix4();

	static Matrix4<T> identity();
	static Matrix4<T> translation(T x, T y, T z);
	static Matrix4<T> scale(T x, T y, T z);
	static Matrix4<T> xRotation(T radians);
	static Matrix4<T> yRotation(T radians);
	static Matrix4<T> zRotation(T radians);
	static Matrix4<T> view(const Vector3f<T> &eye, const Vector3f<T> &forward,
		const Vector3f<T> &right, const Vector3f<T> &up);
	static Matrix4<T> projection(T near, T far, T width, T height);
	static Matrix4<T> perspective(T fovY, T aspect, T near, T far);

	Matrix4<T> operator*(const Matrix4<T> &m) const;
	Vector4f<T> operator*(const Vector4f<T> &f) const;
	std::string toString() const;
};

// The template instantiations are at the end of the .cpp file.
typedef Matrix4<float> Matrix4f;
typedef Matrix4<double> Matrix4d;

#endif
