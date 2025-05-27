#ifndef MATRIX4_H
#define MATRIX4_H

#include <string>
#include <type_traits>

#include "Vector3.h"
#include "Vector4.h"

template<typename T>
class Matrix4
{
public:
	static_assert(std::is_floating_point<T>::value);

	// Column vectors.
	Vector4f<T> x;
	Vector4f<T> y;
	Vector4f<T> z;
	Vector4f<T> w;

	Matrix4(const Vector4f<T> &x, const Vector4f<T> &y, const Vector4f<T> &z,
		const Vector4f<T> &w);
	Matrix4() = default;

	static Matrix4<T> identity();
	static Matrix4<T> translation(T x, T y, T z);
	static Matrix4<T> scale(T x, T y, T z);
	static Matrix4<T> xRotation(T radians);
	static Matrix4<T> yRotation(T radians);
	static Matrix4<T> zRotation(T radians);
	static Matrix4<T> transpose(const Matrix4<T> &m);
	static Matrix4<T> inverse(const Matrix4<T> &m);
	static Matrix4<T> inverseTranslation(const Matrix4<T> &t);
	static Matrix4<T> inverseRotation(const Matrix4<T> &r);
	static Matrix4<T> view(const Vector3f<T> &eye, const Vector3f<T> &forward, const Vector3f<T> &right, const Vector3f<T> &up);
	static Matrix4<T> perspective(T fovY, T aspect, T near, T far);

	Matrix4<T> operator*(const Matrix4<T> &m) const;
	Vector4f<T> operator*(const Vector4f<T> &v) const;
	T getDeterminant() const;

	std::string toString() const;
};

// The template instantiations are at the end of the .cpp file.
using Matrix4f = Matrix4<float>;
using Matrix4d = Matrix4<double>;

#endif
