#include <cmath>

#include "Constants.h"
#include "Matrix4.h"

template <typename T>
Matrix4<T>::Matrix4(const Vector4f<T> &x, const Vector4f<T> &y, const Vector4f<T> &z,
	const Vector4f<T> &w)
	: x(x), y(y), z(z), w(w) { }

template <typename T>
Matrix4<T> Matrix4<T>::identity()
{
	Matrix4<T> m;
	m.x.x = static_cast<T>(1.0);
	m.y.y = static_cast<T>(1.0);
	m.z.z = static_cast<T>(1.0);
	m.w.w = static_cast<T>(1.0);
	return m;
}

template <typename T>
Matrix4<T> Matrix4<T>::translation(T x, T y, T z)
{
	Matrix4<T> m = Matrix4<T>::identity();
	m.w.x = x;
	m.w.y = y;
	m.w.z = z;
	return m;
}

template <typename T>
Matrix4<T> Matrix4<T>::scale(T x, T y, T z)
{
	Matrix4<T> m = Matrix4<T>::identity();
	m.x.x = x;
	m.y.y = y;
	m.z.z = z;
	return m;
}

template <typename T>
Matrix4<T> Matrix4<T>::xRotation(T radians)
{
	Matrix4<T> m = Matrix4<T>::identity();
	T sAngle = static_cast<T>(std::sin(radians));
	T cAngle = static_cast<T>(std::cos(radians));
	m.y.y = cAngle;
	m.y.z = sAngle;
	m.z.y = -sAngle;
	m.z.z = cAngle;
	return m;
}

template <typename T>
Matrix4<T> Matrix4<T>::yRotation(T radians)
{
	Matrix4<T> m = Matrix4<T>::identity();
	T sAngle = static_cast<T>(std::sin(radians));
	T cAngle = static_cast<T>(std::cos(radians));
	m.x.x = cAngle;
	m.x.z = sAngle;
	m.z.x = -sAngle;
	m.z.z = cAngle;
	return m;
}

template <typename T>
Matrix4<T> Matrix4<T>::zRotation(T radians)
{
	Matrix4<T> m = Matrix4<T>::identity();
	T sAngle = static_cast<T>(std::sin(radians));
	T cAngle = static_cast<T>(std::cos(radians));
	m.x.x = cAngle;
	m.x.y = sAngle;
	m.y.x = -sAngle;
	m.y.y = cAngle;
	return m;
}

template <typename T>
Matrix4<T> Matrix4<T>::transpose(const Matrix4<T> &m)
{
	// Flip across diagonal.
	Matrix4<T> newM;
	newM.x = Vector4f<T>(m.x.x, m.y.x, m.z.x, m.w.x);
	newM.y = Vector4f<T>(m.x.y, m.y.y, m.z.y, m.w.y);
	newM.z = Vector4f<T>(m.x.z, m.y.z, m.z.z, m.w.z);
	newM.w = Vector4f<T>(m.x.w, m.y.w, m.z.w, m.w.w);
	return newM;
}

template <typename T>
Matrix4<T> Matrix4<T>::inverseTranslation(const Matrix4<T> &t)
{
	// Undo translation.
	Matrix4<T> m = Matrix4<T>::identity();
	m.w.x = -t.w.x;
	m.w.y = -t.w.y;
	m.w.z = -t.w.z;
	return m;
}

template <typename T>
Matrix4<T> Matrix4<T>::inverseRotation(const Matrix4<T> &r)
{
	// Undo rotation. Mathematically equivalent to transpose, but only need 3x3 here.
	Matrix4<T> m = Matrix4<T>::identity();
	m.x = Vector4f<T>(r.x.x, r.y.x, r.z.x, static_cast<T>(0.0));
	m.y = Vector4f<T>(r.x.y, r.y.y, r.z.y, static_cast<T>(0.0));
	m.z = Vector4f<T>(r.x.z, r.y.z, r.z.z, static_cast<T>(0.0));
	return m;
}

template <typename T>
Matrix4<T> Matrix4<T>::view(const Vector3f<T> &eye, const Vector3f<T> &forward,
	const Vector3f<T> &right, const Vector3f<T> &up)
{
	// [ rx  ux  fx  ex ]    [ rx  ry  rz  -ex ]
	// [ ry  uy  fy  ey ] -> [ ux  uy  uz  -ey ]
	// [ rz  uz  fz  ez ]    [ fx  fy  fz  -ez ]
	// [ 0   0   0   1  ]    [ 0   0   0   1   ]

	Matrix4<T> rotationMat = Matrix4<T>::identity();
	rotationMat.x = Vector4f<T>(right, static_cast<T>(0.0));
	rotationMat.y = Vector4f<T>(up, static_cast<T>(0.0));
	rotationMat.z = Vector4f<T>(forward, static_cast<T>(0.0));
	const Matrix4<T> invRotationMat = Matrix4<T>::inverseRotation(rotationMat);

	const Matrix4<T> translationMat = Matrix4<T>::translation(eye.x, eye.y, eye.z);
	const Matrix4<T> invTranslationMat = Matrix4<T>::inverseTranslation(translationMat);
	return invRotationMat * invTranslationMat;
}

template <typename T>
Matrix4<T> Matrix4<T>::perspective(T fovY, T aspect, T near, T far)
{
	const T zoom = static_cast<T>(1.0 / std::tan((fovY * 0.50) * Constants::DegToRad));
	const T farNearDiff = far - near;

	Matrix4<T> m = Matrix4<T>::identity();
	m.x.x = zoom / aspect;
	m.y.y = zoom;
	m.z.z = -(far + near) / farNearDiff;
	m.z.w = static_cast<T>(-1.0);
	m.w.z = ((static_cast<T>(-2.0) * far) * near) / farNearDiff;
	m.w.w = static_cast<T>(0.0);
	return m;
}

template <typename T>
Matrix4<T> Matrix4<T>::operator*(const Matrix4<T> &m) const
{
	Matrix4<T> p;

	// Column X.
	p.x.x =
		(this->x.x * m.x.x) +
		(this->y.x * m.x.y) +
		(this->z.x * m.x.z) +
		(this->w.x * m.x.w);
	p.x.y =
		(this->x.y * m.x.x) +
		(this->y.y * m.x.y) +
		(this->z.y * m.x.z) +
		(this->w.y * m.x.w);
	p.x.z =
		(this->x.z * m.x.x) +
		(this->y.z * m.x.y) +
		(this->z.z * m.x.z) +
		(this->w.z * m.x.w);
	p.x.w =
		(this->x.w * m.x.x) +
		(this->y.w * m.x.y) +
		(this->z.w * m.x.z) +
		(this->w.w * m.x.w);

	// Column Y.
	p.y.x =
		(this->x.x * m.y.x) +
		(this->y.x * m.y.y) +
		(this->z.x * m.y.z) +
		(this->w.x * m.y.w);
	p.y.y =
		(this->x.y * m.y.x) +
		(this->y.y * m.y.y) +
		(this->z.y * m.y.z) +
		(this->w.y * m.y.w);
	p.y.z =
		(this->x.z * m.y.x) +
		(this->y.z * m.y.y) +
		(this->z.z * m.y.z) +
		(this->w.z * m.y.w);
	p.y.w =
		(this->x.w * m.y.x) +
		(this->y.w * m.y.y) +
		(this->z.w * m.y.z) +
		(this->w.w * m.y.w);

	// Column Z.
	p.z.x =
		(this->x.x * m.z.x) +
		(this->y.x * m.z.y) +
		(this->z.x * m.z.z) +
		(this->w.x * m.z.w);
	p.z.y =
		(this->x.y * m.z.x) +
		(this->y.y * m.z.y) +
		(this->z.y * m.z.z) +
		(this->w.y * m.z.w);
	p.z.z =
		(this->x.z * m.z.x) +
		(this->y.z * m.z.y) +
		(this->z.z * m.z.z) +
		(this->w.z * m.z.w);
	p.z.w =
		(this->x.w * m.z.x) +
		(this->y.w * m.z.y) +
		(this->z.w * m.z.z) +
		(this->w.w * m.z.w);

	// Column W.
	p.w.x =
		(this->x.x * m.w.x) +
		(this->y.x * m.w.y) +
		(this->z.x * m.w.z) +
		(this->w.x * m.w.w);
	p.w.y =
		(this->x.y * m.w.x) +
		(this->y.y * m.w.y) +
		(this->z.y * m.w.z) +
		(this->w.y * m.w.w);
	p.w.z =
		(this->x.z * m.w.x) +
		(this->y.z * m.w.y) +
		(this->z.z * m.w.z) +
		(this->w.z * m.w.w);
	p.w.w =
		(this->x.w * m.w.x) +
		(this->y.w * m.w.y) +
		(this->z.w * m.w.z) +
		(this->w.w * m.w.w);

	return p;
}

template <typename T>
Vector4f<T> Matrix4<T>::operator*(const Vector4f<T> &v) const
{
	const T newX = (this->x.x * v.x) + (this->y.x * v.y) + (this->z.x * v.z) + (this->w.x * v.w);
	const T newY = (this->x.y * v.x) + (this->y.y * v.y) + (this->z.y * v.z) + (this->w.y * v.w);
	const T newZ = (this->x.z * v.x) + (this->y.z * v.y) + (this->z.z * v.z) + (this->w.z * v.w);
	const T newW = (this->x.w * v.x) + (this->y.w * v.y) + (this->z.w * v.z) + (this->w.w * v.w);
	return Vector4f<T>(newX, newY, newZ, newW);
}

template <typename T>
std::string Matrix4<T>::toString() const
{
	return std::string("[") +
		std::to_string(this->x.x) + std::string(", ") +
		std::to_string(this->y.x) + std::string(", ") +
		std::to_string(this->z.x) + std::string(", ") +
		std::to_string(this->w.x) + std::string(",\n ") +

		std::to_string(this->x.y) + std::string(", ") +
		std::to_string(this->y.y) + std::string(", ") +
		std::to_string(this->z.y) + std::string(", ") +
		std::to_string(this->w.y) + std::string(",\n ") +

		std::to_string(this->x.z) + std::string(", ") +
		std::to_string(this->y.z) + std::string(", ") +
		std::to_string(this->z.z) + std::string(", ") +
		std::to_string(this->w.z) + std::string(",\n ") +

		std::to_string(this->x.w) + std::string(", ") +
		std::to_string(this->y.w) + std::string(", ") +
		std::to_string(this->z.w) + std::string(", ") +
		std::to_string(this->w.w) + std::string("]");
}

// Template instantiations.
template class Matrix4<float>;
template class Matrix4<double>;
