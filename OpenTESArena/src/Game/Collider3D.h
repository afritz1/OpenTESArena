#ifndef COLLIDER3D_H
#define COLLIDER3D_H

#include "../Math/Vector2.h"
#include "../Math/Vector3.h"
#include "../Math/Matrix4.h"
#include "../World/VoxelData.h"

class CapsuleCollider3D;
class BoxCollider3D;

class Collider3D
{
public:
	Matrix4<double> Transform;
	~Collider3D();

	virtual bool CheckCollision(const Collider3D& other) = 0;

protected:
	Collider3D(const Matrix4<double> &transform);
	static bool CheckCollisionCapsuleCapsule(const CapsuleCollider3D& A, const CapsuleCollider3D& B);
	static bool CheckCollisionBoxCapsule(const BoxCollider3D& A, const CapsuleCollider3D& B);
	static bool CheckCollisionBoxBox(const BoxCollider3D& A, const BoxCollider3D& B);
};

class CapsuleCollider3D : Collider3D
{
public:
	CapsuleCollider3D(const Matrix4<double> &transform, const double &radius, const double &length);

	double Radius;
	double Length;
protected:
	bool CheckCollision(const Collider3D& other);
private:
};

class BoxCollider3D : Collider3D
{
public:
	BoxCollider3D(const Matrix4<double> &transform, const double &width, const double &height, const double &depth);
	double Width;
	double Height;
	double Depth;
protected:
	bool CheckCollision(const Collider3D& other);
private:
};

#endif