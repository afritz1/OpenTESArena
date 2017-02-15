#include <cmath>

#include "Matrix4.h"

#include "Constants.h"

template <typename T>
Matrix4<T>::Matrix4(const Vector4f<T> &x, const Vector4f<T> &y, const Vector4f<T> &z,
	const Vector4f<T> &w)
{
	this->x[0] = x.x;
	this->x[1] = x.y;
	this->x[2] = x.z;
	this->x[3] = x.w;

	this->y[0] = y.x;
	this->y[1] = y.y;
	this->y[2] = y.z;
	this->y[3] = y.w;

	this->z[0] = z.x;
	this->z[1] = z.y;
	this->z[2] = z.z;
	this->z[3] = z.w;

	this->w[0] = w.x;
	this->w[1] = w.y;
	this->w[2] = w.z;
	this->w[3] = w.w;
}

template <typename T>
Matrix4<T>::Matrix4()
{
	this->x[0] = static_cast<T>(0.0);
	this->x[1] = static_cast<T>(0.0);
	this->x[2] = static_cast<T>(0.0);
	this->x[3] = static_cast<T>(0.0);

	this->y[0] = static_cast<T>(0.0);
	this->y[1] = static_cast<T>(0.0);
	this->y[2] = static_cast<T>(0.0);
	this->y[3] = static_cast<T>(0.0);

	this->z[0] = static_cast<T>(0.0);
	this->z[1] = static_cast<T>(0.0);
	this->z[2] = static_cast<T>(0.0);
	this->z[3] = static_cast<T>(0.0);

	this->w[0] = static_cast<T>(0.0);
	this->w[1] = static_cast<T>(0.0);
	this->w[2] = static_cast<T>(0.0);
	this->w[3] = static_cast<T>(0.0);
}

template <typename T>
Matrix4<T>::~Matrix4()
{

}

template <typename T>
Matrix4<T> Matrix4<T>::identity()
{
	Matrix4<T> m;
	m.x[0] = static_cast<T>(1.0);
	m.y[1] = static_cast<T>(1.0);
	m.z[2] = static_cast<T>(1.0);
	m.w[3] = static_cast<T>(1.0);
	return m;
}

template <typename T>
Matrix4<T> Matrix4<T>::translation(T x, T y, T z)
{
	Matrix4<T> m = Matrix4<T>::identity();
	m.w[0] = x;
	m.w[1] = y;
	m.w[2] = z;
	return m;
}

template <typename T>
Matrix4<T> Matrix4<T>::scale(T x, T y, T z)
{
	Matrix4<T> m = Matrix4<T>::identity();
	m.x[0] = x;
	m.y[1] = y;
	m.z[2] = z;
	return m;
}

template <typename T>
Matrix4<T> Matrix4<T>::xRotation(T radians)
{
	Matrix4<T> m = Matrix4<T>::identity();
	T sAngle = static_cast<T>(std::sin(radians));
	T cAngle = static_cast<T>(std::cos(radians));
	m.y[1] = cAngle;
	m.y[2] = sAngle;
	m.z[1] = -sAngle;
	m.z[2] = cAngle;
	return m;
}

template <typename T>
Matrix4<T> Matrix4<T>::yRotation(T radians)
{
	Matrix4<T> m = Matrix4<T>::identity();
	T sAngle = static_cast<T>(std::sin(radians));
	T cAngle = static_cast<T>(std::cos(radians));
	m.x[0] = cAngle;
	m.x[2] = sAngle;
	m.z[0] = -sAngle;
	m.z[2] = cAngle;
	return m;
}

template <typename T>
Matrix4<T> Matrix4<T>::zRotation(T radians)
{
	Matrix4<T> m = Matrix4<T>::identity();
	T sAngle = static_cast<T>(std::sin(radians));
	T cAngle = static_cast<T>(std::cos(radians));
	m.x[0] = cAngle;
	m.x[1] = sAngle;
	m.y[0] = -sAngle;
	m.y[1] = cAngle;
	return m;
}

template<typename T>
Matrix4<T> Matrix4<T>::view(const Vector3f<T> &eye, const Vector3f<T> &forward,
	const Vector3f<T> &right, const Vector3f<T> &up)
{
	Matrix4<T> rotation = Matrix4<T>::identity();
	rotation.x[0] = right.x;
	rotation.y[0] = right.y;
	rotation.z[0] = right.z;
	rotation.x[1] = up.x;
	rotation.y[1] = up.y;
	rotation.z[1] = up.z;
	rotation.x[2] = -forward.x;
	rotation.y[2] = -forward.y;
	rotation.z[2] = -forward.z;

	Matrix4<T> transpose = Matrix4<T>::translation(-eye.x, -eye.y, -eye.z);

	return rotation * transpose;
}

template <typename T>
Matrix4<T> Matrix4<T>::projection(T near, T far, T width, T height)
{
	Matrix4<T> m = Matrix4<T>::identity();
	m.x[0] = (static_cast<T>(2.0) * near) / width;
	m.y[1] = (static_cast<T>(2.0) * near) / height;
	m.z[2] = -(far + near) / (far - near);
	m.z[3] = static_cast<T>(-1.0);
	m.w[2] = ((static_cast<T>(-2.0) * far) * near) / (far - near);
	m.w[3] = static_cast<T>(0.0);
	return m;
}

template <typename T>
Matrix4<T> Matrix4<T>::perspective(T fovY, T aspect, T near, T far)
{
	T height = (static_cast<T>(2.0) * near) * static_cast<T>(
		std::tan((static_cast<T>(0.5 * PI) * fovY) / static_cast<T>(180.0)));
	T width = height * aspect;
	return Matrix4<T>::projection(near, far, width, height);
}

template <typename T>
Matrix4<T> Matrix4<T>::operator*(const Matrix4<T> &m) const
{
	Matrix4<T> p;

	// Column X.
	p.x[0] =
		(this->x[0] * m.x[0]) +
		(this->y[0] * m.x[1]) +
		(this->z[0] * m.x[2]) +
		(this->w[0] * m.x[3]);
	p.x[1] =
		(this->x[1] * m.x[0]) +
		(this->y[1] * m.x[1]) +
		(this->z[1] * m.x[2]) +
		(this->w[1] * m.x[3]);
	p.x[2] =
		(this->x[2] * m.x[0]) +
		(this->y[2] * m.x[1]) +
		(this->z[2] * m.x[2]) +
		(this->w[2] * m.x[3]);
	p.x[3] =
		(this->x[3] * m.x[0]) +
		(this->y[3] * m.x[1]) +
		(this->z[3] * m.x[2]) +
		(this->w[3] * m.x[3]);

	// Column Y.
	p.y[0] =
		(this->x[0] * m.y[0]) +
		(this->y[0] * m.y[1]) +
		(this->z[0] * m.y[2]) +
		(this->w[0] * m.y[3]);
	p.y[1] =
		(this->x[1] * m.y[0]) +
		(this->y[1] * m.y[1]) +
		(this->z[1] * m.y[2]) +
		(this->w[1] * m.y[3]);
	p.y[2] =
		(this->x[2] * m.y[0]) +
		(this->y[2] * m.y[1]) +
		(this->z[2] * m.y[2]) +
		(this->w[2] * m.y[3]);
	p.y[3] =
		(this->x[3] * m.y[0]) +
		(this->y[3] * m.y[1]) +
		(this->z[3] * m.y[2]) +
		(this->w[3] * m.y[3]);

	// Column Z.
	p.z[0] =
		(this->x[0] * m.z[0]) +
		(this->y[0] * m.z[1]) +
		(this->z[0] * m.z[2]) +
		(this->w[0] * m.z[3]);
	p.z[1] =
		(this->x[1] * m.z[0]) +
		(this->y[1] * m.z[1]) +
		(this->z[1] * m.z[2]) +
		(this->w[1] * m.z[3]);
	p.z[2] =
		(this->x[2] * m.z[0]) +
		(this->y[2] * m.z[1]) +
		(this->z[2] * m.z[2]) +
		(this->w[2] * m.z[3]);
	p.z[3] =
		(this->x[3] * m.z[0]) +
		(this->y[3] * m.z[1]) +
		(this->z[3] * m.z[2]) +
		(this->w[3] * m.z[3]);

	// Column W.
	p.w[0] =
		(this->x[0] * m.w[0]) +
		(this->y[0] * m.w[1]) +
		(this->z[0] * m.w[2]) +
		(this->w[0] * m.w[3]);
	p.w[1] =
		(this->x[1] * m.w[0]) +
		(this->y[1] * m.w[1]) +
		(this->z[1] * m.w[2]) +
		(this->w[1] * m.w[3]);
	p.w[2] =
		(this->x[2] * m.w[0]) +
		(this->y[2] * m.w[1]) +
		(this->z[2] * m.w[2]) +
		(this->w[2] * m.w[3]);
	p.w[3] =
		(this->x[3] * m.w[0]) +
		(this->y[3] * m.w[1]) +
		(this->z[3] * m.w[2]) +
		(this->w[3] * m.w[3]);

	return p;
}

template <typename T>
Vector4f<T> Matrix4<T>::operator*(const Vector4f<T> &f) const
{
	T newX = (this->x[0] * f.x) + (this->y[0] * f.y) +
		(this->z[0] * f.z) + (this->w[0] * f.w);
	T newY = (this->x[1] * f.x) + (this->y[1] * f.y) +
		(this->z[1] * f.z) + (this->w[1] * f.w);
	T newZ = (this->x[2] * f.x) + (this->y[2] * f.y) +
		(this->z[2] * f.z) + (this->w[2] * f.w);
	T newW = (this->x[3] * f.x) + (this->y[3] * f.y) +
		(this->z[3] * f.z) + (this->w[3] * f.w);
	return Vector4f<T>(newX, newY, newZ, newW);
}

template <typename T>
std::string Matrix4<T>::toString() const
{
	return std::string("[") +
		std::to_string(this->x[0]) + std::string(", ") +
		std::to_string(this->y[0]) + std::string(", ") +
		std::to_string(this->z[0]) + std::string(", ") +
		std::to_string(this->w[0]) + std::string(",\n ") +

		std::to_string(this->x[1]) + std::string(", ") +
		std::to_string(this->y[1]) + std::string(", ") +
		std::to_string(this->z[1]) + std::string(", ") +
		std::to_string(this->w[1]) + std::string(",\n ") +

		std::to_string(this->x[2]) + std::string(", ") +
		std::to_string(this->y[2]) + std::string(", ") +
		std::to_string(this->z[2]) + std::string(", ") +
		std::to_string(this->w[2]) + std::string(",\n ") +

		std::to_string(this->x[3]) + std::string(", ") +
		std::to_string(this->y[3]) + std::string(", ") +
		std::to_string(this->z[3]) + std::string(", ") +
		std::to_string(this->w[3]) + std::string("]");
}

// Template instantiations.
template class Matrix4<float>;
template class Matrix4<double>;
