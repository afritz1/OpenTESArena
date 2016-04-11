#include <cmath>

#include "Matrix4.h"

#include "Constants.h"
#include "Float3.h"
#include "Float4.h"

template <typename T>
Matrix4<T>::Matrix4(const Float4<T> &x, const Float4<T> &y, const Float4<T> &z,
	const Float4<T> &w)
{
	this->x[0] = x.getX();
	this->x[1] = x.getY();
	this->x[2] = x.getZ();
	this->x[3] = x.getW();

	this->y[0] = y.getX();
	this->y[1] = y.getY();
	this->y[2] = y.getZ();
	this->y[3] = y.getW();

	this->z[0] = z.getX();
	this->z[1] = z.getY();
	this->z[2] = z.getZ();
	this->z[3] = z.getW();

	this->w[0] = w.getX();
	this->w[1] = w.getY();
	this->w[2] = w.getZ();
	this->w[3] = w.getW();
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
Float4<T> Matrix4<T>::getX() const
{
	return Float4<T>(this->x[0], this->x[1], this->x[2], this->x[3]);
}

template <typename T>
Float4<T> Matrix4<T>::getY() const
{
	return Float4<T>(this->y[0], this->y[1], this->y[2], this->y[3]);
}

template <typename T>
Float4<T> Matrix4<T>::getZ() const
{
	return Float4<T>(this->z[0], this->z[1], this->z[2], this->z[3]);
}

template <typename T>
Float4<T> Matrix4<T>::getW() const
{
	return Float4<T>(this->w[0], this->w[1], this->w[2], this->w[3]);
}

template <typename T>
Matrix4<T> Matrix4<T>::identity()
{
	auto m = Matrix4<T>();
	m.x[0] = static_cast<T>(1.0);
	m.y[1] = static_cast<T>(1.0);
	m.z[2] = static_cast<T>(1.0);
	m.w[3] = static_cast<T>(1.0);
	return m;
}

template <typename T>
Matrix4<T> Matrix4<T>::translation(T x, T y, T z)
{
	auto m = Matrix4<T>::identity();
	m.x[3] = x;
	m.y[3] = y;
	m.z[3] = z;
	return m;
}

template <typename T>
Matrix4<T> Matrix4<T>::scale(T x, T y, T z)
{
	auto m = Matrix4<T>::identity();
	m.x[0] = x;
	m.y[1] = y;
	m.z[2] = z;
	return m;
}

template <typename T>
Matrix4<T> Matrix4<T>::xRotation(T radians)
{
	auto m = Matrix4<T>::identity();
	auto sAngle = static_cast<T>(std::sin(radians));
	auto cAngle = static_cast<T>(std::cos(radians));
	m.y[1] = cAngle;
	m.y[2] = sAngle;
	m.z[1] = -sAngle;
	m.z[2] = cAngle;
	return m;
}

template <typename T>
Matrix4<T> Matrix4<T>::yRotation(T radians)
{
	auto m = Matrix4<T>::identity();
	auto sAngle = static_cast<T>(std::sin(radians));
	auto cAngle = static_cast<T>(std::cos(radians));
	m.x[0] = cAngle;
	m.x[2] = sAngle;
	m.z[0] = -sAngle;
	m.z[2] = cAngle;
	return m;
}

template <typename T>
Matrix4<T> Matrix4<T>::zRotation(T radians)
{
	auto m = Matrix4<T>::identity();
	auto sAngle = static_cast<T>(std::sin(radians));
	auto cAngle = static_cast<T>(std::cos(radians));
	m.x[0] = cAngle;
	m.x[1] = sAngle;
	m.y[0] = -sAngle;
	m.y[1] = cAngle;
	return m;
}

template <typename T>
Matrix4<T> Matrix4<T>::projection(T near, T far, T width, T height)
{
	auto m = Matrix4<T>::identity();
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
	auto height = (static_cast<T>(2.0) * near) * static_cast<T>(
		std::tan((static_cast<T>(0.5 * PI) * fovY) / static_cast<T>(180.0)));
	auto width = height * aspect;
	return Matrix4<T>::projection(near, far, width, height);
}

template <typename T>
Matrix4<T> Matrix4<T>::operator *(const Matrix4<T> &m) const
{
	auto p = Matrix4<T>();

	// Column X
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

	// Column Y
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

	// Column Z
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

	// Column W
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
Float4<T> Matrix4<T>::operator*(const Float4<T> &f) const
{
	auto newX = (this->x[0] * f.getX()) + (this->y[0] * f.getY()) +
		(this->z[0] * f.getZ()) + (this->w[0] * f.getW());
	auto newY = (this->x[1] * f.getX()) + (this->y[1] * f.getY()) +
		(this->z[1] * f.getZ()) + (this->w[1] * f.getW());
	auto newZ = (this->x[2] * f.getX()) + (this->y[2] * f.getY()) +
		(this->z[2] * f.getZ()) + (this->w[2] * f.getW());
	auto newW = (this->x[3] * f.getX()) + (this->y[3] * f.getY()) +
		(this->z[3] * f.getZ()) + (this->w[3] * f.getW());
	return Float4<T>(newX, newY, newZ, newW);
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
