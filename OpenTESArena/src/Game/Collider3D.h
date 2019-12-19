#ifndef COLLIDER3D_H
#define COLLIDER3D_H

#include "../Math/Vector2.h"
#include "../Math/Vector3.h"
#include "../Math/Matrix4.h"
#include "../World/VoxelData.h"

class CapsuleCollider3D;
class AxisAlignedCylinderCollider3D;
class BoxCollider3D;
class QuadCollider3D;

class Collider3D
{
public:
	struct ColliderHit
	{
	public:
		ColliderHit(const Collider3D *a, const Collider3D *b, const Double3 &pointOfImpactOnA, const Double3 &pointOfImpactOnB, const Double3 &normal);
		const Collider3D *A;		// Collider
		const Collider3D *B;		// Other collider
		Double3 PointOfImpactOnA;	// Point of Collision on surface of A
		Double3 PointOfImpactOnB;	// Point of Collision on surface of B
		Double3 Normal;				// Normal at the point of collision on the surface of A
	};

	Matrix4<double> Transform;
	~Collider3D();

	virtual bool CheckCollision(const Collider3D &other, ColliderHit &hit) = 0;

protected:
	Collider3D(const Matrix4d &transform);
	static bool CheckCollisionCapsuleCapsule(const CapsuleCollider3D &A, const CapsuleCollider3D &B, ColliderHit &hit);
	static bool CheckCollisionBoxCapsule(const BoxCollider3D &A, const CapsuleCollider3D &B, ColliderHit &hit);
	static bool CheckCollisionBoxBox(const BoxCollider3D &A, const BoxCollider3D &B, ColliderHit &hit);
	static bool CheckCollisionCapsuleQuad(const CapsuleCollider3D &A, const QuadCollider3D &B, ColliderHit &hit);
	static bool CheckCollisionQuadBox(const QuadCollider3D &A, const BoxCollider3D &B, ColliderHit &hit);
	static bool CheckCollisionCylinderCylinder(const AxisAlignedCylinderCollider3D &A, const AxisAlignedCylinderCollider3D &B, ColliderHit &hit);
	static bool CheckCollisionCylinderCapsule(const AxisAlignedCylinderCollider3D &A, const CapsuleCollider3D &B, ColliderHit &hit);
	static bool CheckCollisionBoxCylinder(const BoxCollider3D &A, const AxisAlignedCylinderCollider3D &B, ColliderHit &hit);
	static bool CheckCollisionCylinderQuad(const AxisAlignedCylinderCollider3D &A, const QuadCollider3D &B, ColliderHit &hit);
};

class CapsuleCollider3D : public Collider3D
{
public:
	CapsuleCollider3D(const Matrix4d &transform, double radius, double length);

	double Radius;
	double Length;
protected:
	bool CheckCollision(const Collider3D &other, ColliderHit &hit);
private:
};

// This class represents a cylinder that's parallel to the Y-axis
class AxisAlignedCylinderCollider3D : public Collider3D
{
public:
	AxisAlignedCylinderCollider3D(const Double3 &center, double radius, double height);

	double Radius;
	double Height;
protected:
	bool CheckCollision(const Collider3D &other, ColliderHit &hit);
};

class BoxCollider3D : public Collider3D
{
public:
	BoxCollider3D(const Matrix4d &transform, double width, double height, double depth);
	double Width;
	double Height;
	double Depth;
protected:
	bool CheckCollision(const Collider3D &other, ColliderHit &hit);
private:
};

class QuadCollider3D : public Collider3D
{
public:
	QuadCollider3D(const Double3 &center, const Double3 &normal, double width, double height);
	Double3 Point;
	Double3 Normal;
	double Width;
	double Height;
protected:
	bool CheckCollision(const Collider3D &other, ColliderHit &hit);
private:
};

#endif