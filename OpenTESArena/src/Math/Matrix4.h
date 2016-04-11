#ifndef MATRIX4_H
#define MATRIX4_H

#include <string>
#include <type_traits>

template <typename T>
class Float4;

template <typename T>
class Matrix4
{
	static_assert(std::is_floating_point<T>::value, "Matrix4<T> must be floating point.");
private:
	// Column vectors.
	T x[4];
	T y[4];
	T z[4];
	T w[4];
public:
	Matrix4(const Float4<T> &x, const Float4<T> &y, const Float4<T> &z,
		const Float4<T> &w);
	Matrix4();
	~Matrix4();

	Float4<T> getX() const;
	Float4<T> getY() const;
	Float4<T> getZ() const;
	Float4<T> getW() const;

	static Matrix4<T> identity();
	static Matrix4<T> translation(T x, T y, T z);
	static Matrix4<T> scale(T x, T y, T z);
	static Matrix4<T> xRotation(T radians);
	static Matrix4<T> yRotation(T radians);
	static Matrix4<T> zRotation(T radians);
	static Matrix4<T> projection(T near, T far, T width, T height);
	static Matrix4<T> perspective(T fovY, T aspect, T near, T far);

	Matrix4<T> operator *(const Matrix4<T> &m) const;
	Float4<T> operator *(const Float4<T> &f) const;
	std::string toString() const;
};

// The template instantiations are at the end of the .cpp file.
typedef Matrix4<float> Matrix4f;
typedef Matrix4<double> Matrix4d;

#endif
