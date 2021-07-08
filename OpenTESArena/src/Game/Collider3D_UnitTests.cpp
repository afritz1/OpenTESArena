#include <string>
#include "Collider3D.h"
#include "../Math/Constants.h"
#include "components/debug/Debug.h"

inline bool CloseEnough(const Double3 &A, const Double3 &B)
{
	return (A - B).lengthSquared() < Constants::Epsilon;
}

#pragma region Unit Tests - AABB -> AABB

void UnitTestAABB_AABB_NoCollision_PosX()
{
	BoxCollider3D A(Matrix4d::identity(), 2, 2, 2);
	BoxCollider3D B(Matrix4d::translation(2.01, 0, 0), 2, 2, 2);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);
	DebugAssertMsg(!A.CheckCollision(B, hit), "Failed AABB_AABB_NoCollision_X");
	DebugAssertMsg(!B.CheckCollision(A, hit), "Failed AABB_AABB_NoCollision_X");
}

void UnitTestAABB_AABB_NoCollision_NegX()
{
	BoxCollider3D A(Matrix4d::identity(), 2, 2, 2);
	BoxCollider3D B(Matrix4d::translation(-2.01, 0, 0), 2, 2, 2);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);
	DebugAssertMsg(!A.CheckCollision(B, hit), "Failed AABB_AABB_NoCollision_X");
	DebugAssertMsg(!B.CheckCollision(A, hit), "Failed AABB_AABB_NoCollision_X");
}

void UnitTestAABB_AABB_NoCollision_PosY()
{
	BoxCollider3D A(Matrix4d::identity(), 2, 2, 2);
	BoxCollider3D B(Matrix4d::translation(0, 2.01, 0), 2, 2, 2);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);
	DebugAssertMsg(!A.CheckCollision(B, hit), "Failed AABB_AABB_NoCollision_X");
	DebugAssertMsg(!B.CheckCollision(A, hit), "Failed AABB_AABB_NoCollision_X");
}

void UnitTestAABB_AABB_NoCollision_NegY()
{
	BoxCollider3D A(Matrix4d::identity(), 2, 2, 2);
	BoxCollider3D B(Matrix4d::translation(0, -2.01, 0), 2, 2, 2);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);
	DebugAssertMsg(!A.CheckCollision(B, hit), "Failed AABB_AABB_NoCollision_X");
	DebugAssertMsg(!B.CheckCollision(A, hit), "Failed AABB_AABB_NoCollision_X");
}

void UnitTestAABB_AABB_NoCollision_PosZ()
{
	BoxCollider3D A(Matrix4d::identity(), 2, 2, 2);
	BoxCollider3D B(Matrix4d::translation(0, 0, 2.01), 2, 2, 2);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);
	DebugAssertMsg(!A.CheckCollision(B, hit), "Failed AABB_AABB_NoCollision_X");
	DebugAssertMsg(!B.CheckCollision(A, hit), "Failed AABB_AABB_NoCollision_X");
}

void UnitTestAABB_AABB_NoCollision_NegZ()
{
	BoxCollider3D A(Matrix4d::identity(), 2, 2, 2);
	BoxCollider3D B(Matrix4d::translation(0, 0, -2.01), 2, 2, 2);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);
	DebugAssertMsg(!A.CheckCollision(B, hit), "Failed AABB_AABB_NoCollision_X");
	DebugAssertMsg(!B.CheckCollision(A, hit), "Failed AABB_AABB_NoCollision_X");
}

#pragma endregion Unit Tests - AABB -> AABB

#pragma region Unit Tests - AABB -> Axis Aligned Cylinder

#pragma region AABB -> Axis Aligned Cylinder PosY Tests

void UnitTestAABB_Cylinder_Above()
{
	BoxCollider3D A(Matrix4d::translation(0.5, -0.78, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(0.5, 0.01, 0.5), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(!A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(!B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
}

void UnitTestAABB_Cylinder_RestingOnTop()
{
	BoxCollider3D A(Matrix4d::translation(0.5, -0.78, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(0.5, 0, 0.5), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	// Check the values of hit
	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.5, 0, 0.5)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.5, 0, 0.5)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitY, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	// Check the values of hit
	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.5, 0, 0.5)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.5, 0, 0.5)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitY, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
}

void UnitTestAABB_Cylinder_InterpenetratingTop()
{
	BoxCollider3D A(Matrix4d::translation(0.5, -0.78, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(0.5, -0.01, 0.5), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.5, 0, 0.5)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.5, -0.01, 0.5)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitY, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.5, -0.01, 0.5)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.5, 0, 0.5)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitY, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
}

void UnitTestAABB_Cylinder_InterpenetratingTop_PosXPosZCorner()
{
	BoxCollider3D A(Matrix4d::translation(0.5, -0.78, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(1.05, -0.01, 1.05), 0.1, 0.5);

	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(1, 0, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(1, -0.01, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(1, -0.01, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(1, 0, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));
}

void UnitTestAABB_Cylinder_InterpenetratingTop_PosZEdge_NearPosX()
{
	BoxCollider3D A(Matrix4d::translation(0.5, -0.78, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(0.99, -0.01, 1.05), 0.1, 0.5);

	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.99, 0, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.99, -0.01, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.99, -0.01, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.99, 0, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));
}

void UnitTestAABB_Cylinder_InterpenetratingTop_PosZEdge()
{
	BoxCollider3D A(Matrix4d::translation(0.5, -0.78, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(0.5, -0.01, 1.05), 0.1, 0.5);

	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.5, 0, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.5, -0.01, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.5, -0.01, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.5, 0, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));
}

void UnitTestAABB_Cylinder_InterpenetratingTop_PosZEdge_NearNegX()
{
	BoxCollider3D A(Matrix4d::translation(0.5, -0.78, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(0.01, -0.01, 1.05), 0.1, 0.5);

	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.01, 0, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.01, -0.01, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.01, -0.01, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.01, 0, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));
}

void UnitTestAABB_Cylinder_InterpenetratingTop_NegXPosZCorner()
{
	BoxCollider3D A(Matrix4d::translation(0.5, -0.78, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(-0.05, -0.01, 1.05), 0.1, 0.5);

	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0, 0, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0, -0.01, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0, -0.01, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0, 0, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));
}

void UnitTestAABB_Cylinder_InterpenetratingTop_PosXEdge_NearPosZ()
{
	BoxCollider3D A(Matrix4d::translation(0.5, -0.78, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(1.05, -0.01, 0.99), 0.1, 0.5);

	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(1, 0, 0.99)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(1, -0.01, 0.99)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(1, -0.01, 0.99)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(1, 0, 0.99)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));
}

void UnitTestAABB_Cylinder_InterpenetratingTop_PosXEdge()
{
	BoxCollider3D A(Matrix4d::translation(0.5, -0.78, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(1.05, -0.01, 0.5), 0.1, 0.5);

	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(1, 0, 0.5)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(1, -0.01, 0.5)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(1, -0.01, 0.5)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(1, 0, 0.5)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));
}

void UnitTestAABB_Cylinder_InterpenetratingTop_PosXEdge_NearNegZ()
{
	BoxCollider3D A(Matrix4d::translation(0.5, -0.78, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(1.05, -0.01, 0.01), 0.1, 0.5);

	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(1, 0, 0.01)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(1, -0.01, 0.01)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(1, -0.01, 0.01)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(1, 0, 0.01)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));
}

void UnitTestAABB_Cylinder_InterpenetratingTop_PosXNegZCorner()
{
	BoxCollider3D A(Matrix4d::translation(0.5, -0.78, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(1.05, -0.01, -0.05), 0.1, 0.5);

	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(1, 0, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(1, -0.01, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(1, -0.01, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(1, 0, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));
}

void UnitTestAABB_Cylinder_InterpenetratingTop_NegZEdge_NearPosX()
{
	BoxCollider3D A(Matrix4d::translation(0.5, -0.78, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(0.99, -0.01, -0.05), 0.1, 0.5);

	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.99, 0, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.99, -0.01, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.99, -0.01, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.99, 0, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));
}

void UnitTestAABB_Cylinder_InterpenetratingTop_NegZEdge()
{
	BoxCollider3D A(Matrix4d::translation(0.5, -0.78, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(0.5, -0.01, -0.05), 0.1, 0.5);

	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.5, 0, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.5, -0.01, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.5, -0.01, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.5, 0, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));
}

void UnitTestAABB_Cylinder_InterpenetratingTop_NegZEdge_NearNegX()
{
	BoxCollider3D A(Matrix4d::translation(0.5, -0.78, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(0.01, -0.01, -0.05), 0.1, 0.5);

	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.01, 0, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.01, -0.01, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.01, -0.01, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.01, 0, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));
}

void UnitTestAABB_Cylinder_InterpenetratingTop_NegXNegZCorner()
{
	BoxCollider3D A(Matrix4d::translation(0.5, -0.78, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(-0.05, -0.01, -0.05), 0.1, 0.5);

	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0, 0, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0, -0.01, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0, -0.01, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0, 0, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));
}

void UnitTestAABB_Cylinder_InterpenetratingTop_NegXEdge_NearPosZ()
{
	BoxCollider3D A(Matrix4d::translation(0.5, -0.78, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(-0.05, -0.01, 0.99), 0.1, 0.5);

	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0, 0, 0.99)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0, -0.01, 0.99)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0, -0.01, 0.99)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0, 0, 0.99)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));
}

void UnitTestAABB_Cylinder_InterpenetratingTop_NegXEdge()
{
	BoxCollider3D A(Matrix4d::translation(0.5, -0.78, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(-0.05, -0.01, 0.5), 0.1, 0.5);

	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0, 0, 0.5)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0, -0.01, 0.5)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0, -0.01, 0.5)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0, 0, 0.5)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));
}

void UnitTestAABB_Cylinder_InterpenetratingTop_NegXEdge_NearNegZ()
{
	BoxCollider3D A(Matrix4d::translation(0.5, -0.78, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(-0.05, -0.01, 0.01), 0.1, 0.5);

	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0, 0, 0.01)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0, -0.01, 0.01)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0, -0.01, 0.01)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0, 0, 0.01)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));
}

#pragma endregion AABB -> Axis Aligned Cylinder PosY Tests

#pragma region AABB -> Axis Aligned Cylinder NegY Tests

void UnitTestAABB_Cylinder_InterpenetratingBottom_PosXPosZCorner()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(1.05, -0.49, 1.05), 0.1, 0.5);

	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(1, 0, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(1, 0.01, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(1, 0.01, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(1, 0, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));
}

void UnitTestAABB_Cylinder_InterpenetratingBottom_PosZEdge_NearPosX()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(0.99, -0.49, 1.05), 0.1, 0.5);

	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.99, 0, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.99, 0.01, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.99, 0.01, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.99, 0, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));
}

void UnitTestAABB_Cylinder_InterpenetratingBottom_PosZEdge()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(0.5, -0.49, 1.05), 0.1, 0.5);

	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.5, 0, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.5, 0.01, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.5, 0.01, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.5, 0, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));
}

void UnitTestAABB_Cylinder_InterpenetratingBottom_PosZEdge_NearNegX()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(0.01, -0.49, 1.05), 0.1, 0.5);

	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.01, 0, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.01, 0.01, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.01, 0.01, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.01, 0, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));
}

void UnitTestAABB_Cylinder_InterpenetratingBottom_NegXPosZCorner()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(-0.05, -0.49, 1.05), 0.1, 0.5);

	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0, 0, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0, 0.01, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0, 0.01, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0, 0, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));
}

void UnitTestAABB_Cylinder_InterpenetratingBottom_PosXEdge_NearPosZ()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(1.05, -0.49, 0.99), 0.1, 0.5);

	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(1, 0, 0.99)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(1, 0.01, 0.99)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(1, 0.01, 0.99)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(1, 0, 0.99)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));
}

void UnitTestAABB_Cylinder_InterpenetratingBottom_PosXEdge()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(1.05, -0.49, 0.5), 0.1, 0.5);

	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(1, 0, 0.5)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(1, 0.01, 0.5)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(1, 0.01, 0.5)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(1, 0, 0.5)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));
}

void UnitTestAABB_Cylinder_InterpenetratingBottom_PosXEdge_NearNegZ()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(1.05, -0.49, 0.01), 0.1, 0.5);

	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(1, 0, 0.01)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(1, 0.01, 0.01)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(1, 0.01, 0.01)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(1, 0, 0.01)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));
}

void UnitTestAABB_Cylinder_InterpenetratingBottom_PosXNegZCorner()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(1.05, -0.49, -0.05), 0.1, 0.5);

	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(1, 0, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(1, 0.01, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(1, 0.01, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(1, 0, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));
}

void UnitTestAABB_Cylinder_InterpenetratingBottom_NegZEdge_NearPosX()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(0.99, -0.49, -0.05), 0.1, 0.5);

	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.99, 0, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.99, 0.01, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.99, 0.01, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.99, 0, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));
}

void UnitTestAABB_Cylinder_InterpenetratingBottom_NegZEdge()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(0.5, -0.49, -0.05), 0.1, 0.5);

	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.5, 0, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.5, 0.01, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.5, 0.01, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.5, 0, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));
}

void UnitTestAABB_Cylinder_InterpenetratingBottom_NegZEdge_NearNegX()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(0.01, -0.49, -0.05), 0.1, 0.5);

	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.01, 0, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.01, 0.01, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.01, 0.01, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.01, 0, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));
}

void UnitTestAABB_Cylinder_InterpenetratingBottom_NegXNegZCorner()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(-0.05, -0.49, -0.05), 0.1, 0.5);

	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0, 0, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0, 0.01, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0, 0.01, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0, 0, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));
}

void UnitTestAABB_Cylinder_InterpenetratingBottom_NegXEdge_NearPosZ()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(-0.05, -0.49, 0.99), 0.1, 0.5);

	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0, 0, 0.99)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0, 0.01, 0.99)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0, 0.01, 0.99)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0, 0, 0.99)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));
}

void UnitTestAABB_Cylinder_InterpenetratingBottom_NegXEdge()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(-0.05, -0.49, 0.5), 0.1, 0.5);

	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0, 0, 0.5)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0, 0.01, 0.5)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0, 0.01, 0.5)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0, 0, 0.5)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));
}

void UnitTestAABB_Cylinder_InterpenetratingBottom_NegXEdge_NearNegZ()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(-0.05, -0.49, 0.01), 0.1, 0.5);

	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0, 0, 0.01)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0, 0.01, 0.01)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0, 0.01, 0.01)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA: incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0, 0, 0.01)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB: incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitY,
		std::string("Failed ") + __FUNCTION__ + std::string(": hit.Normal: incorrect value"));
}

void UnitTestAABB_Cylinder_InterpenetratingBottom()
{
	BoxCollider3D A(Matrix4d::translation(0.5, -0.78, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(0.5, -1, 0.5), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.5, -0.78, 0.5)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.5, -0.5, 0.5)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitY, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.5, -0.5, 0.5)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.5, -0.78, 0.5)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitY, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
}

void UnitTestAABB_Cylinder_TouchingBottom()
{
	BoxCollider3D A(Matrix4d::translation(0.5, -0.78, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(0.5, -1.28, 0.5), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.5, -0.78, 0.5)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.5, -0.78, 0.5)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitY, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.5, -0.78, 0.5)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.5, -0.78, 0.5)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitY, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
}

void UnitTestAABB_Cylinder_Below()
{
	BoxCollider3D A(Matrix4d::translation(0.5, -0.78, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(0.5, -2, 0.5), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(!A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(!B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
}

#pragma endregion AABB -> Axis Aligned Cylinder NegY Tests

#pragma region AABB -> Axis Aligned Cylinder PosX Tests

void UnitTestAABB_Cylinder_PosX_Interpenetrating()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(1.05, 0, 0.5), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(1, 0.25, 0.5)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.95, 0.25, 0.5)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitX, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.95, 0.25, 0.5)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(1, 0.25, 0.5)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitX, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
}

void UnitTestAABB_Cylinder_PosX_Touching()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(1.1, 0, 0.5), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(1, 0.25, 0.5)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(1, 0.25, 0.5)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitX, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(1, 0.25, 0.5)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(1, 0.25, 0.5)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitX, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
}

void UnitTestAABB_Cylinder_PosX_NoCollision()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(1.11, 0, 0.5), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(!A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(!B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
}

void UnitTestAABB_Cylinder_PosX_NearPosXPosZCorner_Interpenetrating()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(1.05, 0, 0.99), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(1, 0.25, 0.99)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.95, 0.25, 0.99)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitX, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.95, 0.25, 0.99)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(1, 0.25, 0.99)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitX, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
}

void UnitTestAABB_Cylinder_PosX_NearPosXPosZCorner_Touching()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(1.1, 0, 0.99), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(1, 0.25, 0.99)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(1, 0.25, 0.99)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitX, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(1, 0.25, 0.99)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(1, 0.25, 0.99)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitX, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
}

void UnitTestAABB_Cylinder_PosX_NearPosXPosZCorner_NoCollision()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(1.11, 0, 0.99), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(!A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(!B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
}

void UnitTestAABB_Cylinder_PosX_NearPosXNegZCorner_Interpenetrating()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(1.05, 0, 0.01), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(1, 0.25, 0.01)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.95, 0.25, 0.01)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitX, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.95, 0.25, 0.01)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(1, 0.25, 0.01)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitX, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
}

void UnitTestAABB_Cylinder_PosX_NearPosXNegZCorner_Touching()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(1.1, 0, 0.01), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(1, 0.25, 0.01)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(1, 0.25, 0.01)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitX, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(1, 0.25, 0.01)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(1, 0.25, 0.01)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitX, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
}

void UnitTestAABB_Cylinder_PosX_NearPosXNegZCorner_NoCollision()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(-0.11, 0, 0.01), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(!A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(!B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
}

#pragma endregion AABB -> Axis Aligned Cylinder PosX Tests

#pragma region AABB -> Axis Aligned Cylinder NegX Tests

void UnitTestAABB_Cylinder_NegX_Interpenetrating()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(-0.05, 0, 0.5), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0, 0.25, 0.5)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.05, 0.25, 0.5)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitX, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.05, 0.25, 0.5)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0, 0.25, 0.5)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitX, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
}

void UnitTestAABB_Cylinder_NegX_Touching()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(-0.1, 0, 0.5), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0, 0.25, 0.5)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0, 0.25, 0.5)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitX, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0, 0.25, 0.5)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0, 0.25, 0.5)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitX, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
}

void UnitTestAABB_Cylinder_NegX_NoCollision()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(-0.11, 0, 0.5), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(!A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(!B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
}

void UnitTestAABB_Cylinder_NegX_NearNegXPosZCorner_Interpenetrating()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(-0.05, 0, 0.99), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0, 0.25, 0.99)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.05, 0.25, 0.99)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitX, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.05, 0.25, 0.99)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0, 0.25, 0.99)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitX, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
}

void UnitTestAABB_Cylinder_NegX_NearNegXPosZCorner_Touching()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(-0.1, 0, 0.99), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0, 0.25, 0.99)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0, 0.25, 0.99)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitX, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0, 0.25, 0.99)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0, 0.25, 0.99)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitX, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
}

void UnitTestAABB_Cylinder_NegX_NearNegXPosZCorner_NoCollision()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(-0.11, 0, 0.99), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(!A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(!B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
}

void UnitTestAABB_Cylinder_NegX_NearNegXNegZCorner_Interpenetrating()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(-0.05, 0, 0.01), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0, 0.25, 0.01)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.05, 0.25, 0.01)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitX, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.05, 0.25, 0.01)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0, 0.25, 0.01)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitX, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
}

void UnitTestAABB_Cylinder_NegX_NearNegXNegZCorner_Touching()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(-0.1, 0, 0.01), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0, 0.25, 0.01)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0, 0.25, 0.01)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitX, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0, 0.25, 0.01)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0, 0.25, 0.01)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitX, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
}

void UnitTestAABB_Cylinder_NegX_NearNegXNegZCorner_NoCollision()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(-0.11, 0, 0.01), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(!A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(!B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
}

#pragma endregion AABB -> Axis Aligned Cylinder NegX Tests

#pragma region AABB -> Axis Aligned Cylinder PosZ Tests

void UnitTestAABB_Cylinder_PosZ_Interpenetrating()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(0.5, 0, 1.05), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.5, 0.25, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.5, 0.25, 0.95)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitZ, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.5, 0.25, 0.95)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.5, 0.25, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitZ, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
}

void UnitTestAABB_Cylinder_PosZ_Touching()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(0.5, 0, 1.1), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.5, 0.25, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.5, 0.25, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitZ, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.5, 0.25, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.5, 0.25, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitZ, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
}

void UnitTestAABB_Cylinder_PosZ_NoCollision()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(0.5, 0, 1.11), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(!A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(!B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
}

void UnitTestAABB_Cylinder_PosZ_NearPosXPosZCorner_Interpenetrating()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(0.99, 0, 1.05), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.99, 0.25, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.99, 0.25, 0.95)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitZ, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.99, 0.25, 0.95)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.99, 0.25, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitZ, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
}

void UnitTestAABB_Cylinder_PosZ_NearPosXPosZCorner_Touching()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(0.99, 0, 1.1), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.99, 0.25, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.99, 0.25, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitZ, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.99, 0.25, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.99, 0.25, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitZ, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
}

void UnitTestAABB_Cylinder_PosZ_NearPosXPosZCorner_NoCollision()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(0.99, 0, 1.11), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(!A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(!B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
}

void UnitTestAABB_Cylinder_PosZ_NearNegXPosZCorner_Interpenetrating()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(0.01, 0, 1.05), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.01, 0.25, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.01, 0.25, 0.95)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitZ, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.01, 0.25, 0.95)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.01, 0.25, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitZ, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
}

void UnitTestAABB_Cylinder_PosZ_NearNegXPosZCorner_Touching()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(0.01, 0, 1.1), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.01, 0.25, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.01, 0.25, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitZ, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.01, 0.25, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.01, 0.25, 1)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitZ, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
}

void UnitTestAABB_Cylinder_PosZ_NearNegXPosZCorner_NoCollision()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(0.01, 0, 1.11), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(!A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(!B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
}

#pragma endregion AABB -> Axis Aligned Cylinder PosZ Tests

#pragma region AABB -> Axis Aligned Cylinder NegZ Tests

void UnitTestAABB_Cylinder_NegZ_Interpenetrating()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(0.5, 0, -0.05), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.5, 0.25, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.5, 0.25, 0.05)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitZ, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.5, 0.25, 0.05)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.5, 0.25, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitZ, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
}

void UnitTestAABB_Cylinder_NegZ_Touching()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(0.5, 0, -0.1), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.5, 0.25, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.5, 0.25, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitZ, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.5, 0.25, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.5, 0.25, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitZ, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
}

void UnitTestAABB_Cylinder_NegZ_NoCollision()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(0.5, 0, -0.11), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(!A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(!B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
}

void UnitTestAABB_Cylinder_NegZ_NearPosXNegZCorner_Interpenetrating()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(0.99, 0, -0.05), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.99, 0.25, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.99, 0.25, 0.05)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitZ, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.99, 0.25, 0.05)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.99, 0.25, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitZ, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
}

void UnitTestAABB_Cylinder_NegZ_NearPosXNegZCorner_Touching()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(0.99, 0, -0.1), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.99, 0.25, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.99, 0.25, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitZ, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.99, 0.25, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.99, 0.25, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitZ, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
}

void UnitTestAABB_Cylinder_NegZ_NearPosXNegZCorner_NoCollision()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(0.99, 0, -0.11), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(!A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(!B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
}

void UnitTestAABB_Cylinder_NegZ_NearNegXNegZCorner_Interpenetrating()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(0.01, 0, -0.05), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.01, 0.25, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.01, 0.25, 0.05)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitZ, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.01, 0.25, 0.05)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.01, 0.25, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitZ, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
}

void UnitTestAABB_Cylinder_NegZ_NearNegXNegZCorner_Touching()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(0.01, 0, -0.1), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.01, 0.25, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.01, 0.25, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitZ, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.01, 0.25, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.01, 0.25, 0)).lengthSquared() < Constants::Epsilon,
		std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitZ, std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
}

void UnitTestAABB_Cylinder_NegZ_NearNegXNegZCorner_NoCollision()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(0.01, 0, -0.11), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(!A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(!B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
}

#pragma endregion AABB -> Axis Aligned Cylinder NegZ Tests

#pragma region AABB -> Axis Aligned Cylinder Corner Tests

void UnitTestAABB_Cylinder_PosXPosZCorner_Interpenetrating()
{
	double r = 1.05 - (0.05 * sqrt(2));
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(1.05, 0, 1.05), 0.1, 0.5);

	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg(CloseEnough(hit.PointOfImpactOnA, Double3(1, 0.25, 1)), std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(CloseEnough(hit.PointOfImpactOnB, Double3(r, 0.25, r)), std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == (Double3::UnitX + Double3::UnitZ).normalized(), std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg(CloseEnough(hit.PointOfImpactOnA, Double3(r, 0.25, r)), std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(CloseEnough(hit.PointOfImpactOnB, Double3(1, 0.25, 1)), std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(hit.Normal == -(Double3::UnitX + Double3::UnitZ).normalized(), std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
}

void UnitTestAABB_Cylinder_PosXPosZCorner_NoCollision()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(1.1, 0, 1.1), 0.1, 0.5);

	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(!A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(!B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
}

void UnitTestAABB_Cylinder_PosXNegZCorner_Interpenetrating()
{
	double r = 1.05 - (0.05 * sqrt(2));
	double s = -0.05 + (0.05 * sqrt(2));
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(1.05, 0, -0.05), 0.1, 0.5);

	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg(CloseEnough(hit.PointOfImpactOnA, Double3(1, 0.25, 0)), std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(CloseEnough(hit.PointOfImpactOnB, Double3(r, 0.25, s)), std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(CloseEnough(hit.Normal, (Double3::UnitX - Double3::UnitZ).normalized()), std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg(CloseEnough(hit.PointOfImpactOnA, Double3(r, 0.25, s)), std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(CloseEnough(hit.PointOfImpactOnB, Double3(1, 0.25, 0)), std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(CloseEnough(hit.Normal, (Double3::UnitZ - Double3::UnitX).normalized()), std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
}

void UnitTestAABB_Cylinder_PosXNegZCorner_NoCollision()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(1.1, 0, -0.1), 0.1, 0.5);

	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(!A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(!B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
}

void UnitTestAABB_Cylinder_NegXPosZCorner_Interpenetrating()
{
	double r = 1.05 - (0.05 * sqrt(2));
	double s = -0.05 + (0.05 * sqrt(2));
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(-0.05, 0, 1.05), 0.1, 0.5);

	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg(CloseEnough(hit.PointOfImpactOnA, Double3(0, 0.25, 1)), std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(CloseEnough(hit.PointOfImpactOnB, Double3(s, 0.25, r)), std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(CloseEnough(hit.Normal, (Double3::UnitZ - Double3::UnitX).normalized()), std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg(CloseEnough(hit.PointOfImpactOnA, Double3(s, 0.25, r)), std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(CloseEnough(hit.PointOfImpactOnB, Double3(0, 0.25, 1)), std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(CloseEnough(hit.Normal, (Double3::UnitX - Double3::UnitZ).normalized()), std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
}

void UnitTestAABB_Cylinder_NegXPosZCorner_NoCollision()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(-0.1, 0, 1.1), 0.1, 0.5);

	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(!A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(!B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
}

void UnitTestAABB_Cylinder_NegXNegZCorner_Interpenetrating()
{
	double s = -0.05 + (0.05 * sqrt(2));
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(-0.05, 0, -0.05), 0.1, 0.5);

	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg(CloseEnough(hit.PointOfImpactOnA, Double3(0, 0.25, 0)), std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(CloseEnough(hit.PointOfImpactOnB, Double3(s, 0.25, s)), std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(CloseEnough(hit.Normal, -(Double3::UnitZ + Double3::UnitX).normalized()), std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));

	DebugAssertMsg(B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));

	DebugAssertMsg(hit.A == static_cast<const Collider3D *>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(hit.B == static_cast<const Collider3D *>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg(CloseEnough(hit.PointOfImpactOnA, Double3(s, 0.25, s)), std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(CloseEnough(hit.PointOfImpactOnB, Double3(0, 0.25, 0)), std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
	DebugAssertMsg(CloseEnough(hit.Normal, (Double3::UnitX + Double3::UnitZ).normalized()), std::string("Failed ") + __FUNCTION__ + std::string(": incorrect value"));
}

void UnitTestAABB_Cylinder_NegXNegZCorner_NoCollision()
{
	BoxCollider3D A(Matrix4d::translation(0.5, 0, 0.5), 1, 0.78, 1);
	AxisAlignedCylinderCollider3D B(Double3(-0.1, 0, -0.1), 0.1, 0.5);

	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(!A.CheckCollision(B, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(!B.CheckCollision(A, hit), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
}

#pragma endregion AABB -> Axis Aligned Cylinder Corner Tests

#pragma endregion Unit Tests - AABB -> Axis Aligned Cylinder

#pragma region Unit Tests - Axis Aligned Cylinder -> Axis Aligned Cylinder

void UnitTestCylinder_Cylinder_PosX_Interpenetrating()
{
	AxisAlignedCylinderCollider3D A(Double3(0, 0, 0), 0.1, 0.5);
	AxisAlignedCylinderCollider3D B(Double3(0.15, 0, 0), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(static_cast<const Collider3D &>(B), hit), std::string("Failure ") + __FUNCTION__ + std::string(": Invalid Return value"));
	DebugAssertMsg(static_cast<const Collider3D *>(&A) == hit.A, std::string("Failure ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(static_cast<const Collider3D *>(&B) == hit.B, std::string("Failure ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.1, 0.25, 0)).lengthSquared() < Constants::Epsilon, std::string("Failure ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.05, 0.25, 0)).lengthSquared() < Constants::Epsilon, std::string("Failure ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitX, std::string("Failure ") + __FUNCTION__ + std::string(": hit.Normal incorrect value"));

	DebugAssertMsg(B.CheckCollision(static_cast<const Collider3D &>(A), hit), std::string("Failure ") + __FUNCTION__ + std::string(": Invalid Return value"));
	DebugAssertMsg(static_cast<const Collider3D *>(&B) == hit.A, std::string("Failure ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(static_cast<const Collider3D *>(&A) == hit.B, std::string("Failure ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.05, 0.25, 0)).lengthSquared() < Constants::Epsilon, std::string("Failure ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.1, 0.25, 0)).lengthSquared() < Constants::Epsilon, std::string("Failure ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitX, std::string("Failure ") + __FUNCTION__ + std::string(": hit.Normal incorrect value"));
}

void UnitTestCylinder_Cylinder_PosZ_Interpenetrating()
{
	AxisAlignedCylinderCollider3D A(Double3(0, 0, 0), 0.1, 0.5);
	AxisAlignedCylinderCollider3D B(Double3(0, 0, 0.15), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(static_cast<const Collider3D &>(B), hit), std::string("Failure ") + __FUNCTION__ + std::string(": Invalid Return value"));
	DebugAssertMsg(static_cast<const Collider3D *>(&A) == hit.A, std::string("Failure ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(static_cast<const Collider3D *>(&B) == hit.B, std::string("Failure ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0, 0.25, 0.1)).lengthSquared() < Constants::Epsilon, std::string("Failure ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0, 0.25, 0.05)).lengthSquared() < Constants::Epsilon, std::string("Failure ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitZ, std::string("Failure ") + __FUNCTION__ + std::string(": hit.Normal incorrect value"));

	DebugAssertMsg(B.CheckCollision(static_cast<const Collider3D &>(A), hit), std::string("Failure ") + __FUNCTION__ + std::string(": Invalid Return value"));
	DebugAssertMsg(static_cast<const Collider3D *>(&B) == hit.A, std::string("Failure ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(static_cast<const Collider3D *>(&A) == hit.B, std::string("Failure ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0, 0.25, 0.05)).lengthSquared() < Constants::Epsilon, std::string("Failure ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0, 0.25, 0.1)).lengthSquared() < Constants::Epsilon, std::string("Failure ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitZ, std::string("Failure ") + __FUNCTION__ + std::string(": hit.Normal incorrect value"));
}

void UnitTestCylinder_Cylinder_NegX_Interpenetrating()
{
	AxisAlignedCylinderCollider3D A(Double3(0, 0, 0), 0.1, 0.5);
	AxisAlignedCylinderCollider3D B(Double3(-0.15, 0, 0), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(static_cast<const Collider3D &>(B), hit), std::string("Failure ") + __FUNCTION__ + std::string(": Invalid Return value"));
	DebugAssertMsg(static_cast<const Collider3D *>(&A) == hit.A, std::string("Failure ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(static_cast<const Collider3D *>(&B) == hit.B, std::string("Failure ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(-0.1, 0.25, 0)).lengthSquared() < Constants::Epsilon, std::string("Failure ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(-0.05, 0.25, 0)).lengthSquared() < Constants::Epsilon, std::string("Failure ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitX, std::string("Failure ") + __FUNCTION__ + std::string(": hit.Normal incorrect value"));

	DebugAssertMsg(B.CheckCollision(static_cast<const Collider3D &>(A), hit), std::string("Failure ") + __FUNCTION__ + std::string(": Invalid Return value"));
	DebugAssertMsg(static_cast<const Collider3D *>(&B) == hit.A, std::string("Failure ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(static_cast<const Collider3D *>(&A) == hit.B, std::string("Failure ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(-0.05, 0.25, 0)).lengthSquared() < Constants::Epsilon, std::string("Failure ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(-0.1, 0.25, 0)).lengthSquared() < Constants::Epsilon, std::string("Failure ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitX, std::string("Failure ") + __FUNCTION__ + std::string(": hit.Normal incorrect value"));
}

void UnitTestCylinder_Cylinder_NegZ_Interpenetrating()
{
	AxisAlignedCylinderCollider3D A(Double3(0, 0, 0), 0.1, 0.5);
	AxisAlignedCylinderCollider3D B(Double3(0, 0, -0.15), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(static_cast<const Collider3D &>(B), hit), std::string("Failure ") + __FUNCTION__ + std::string(": Invalid Return value"));
	DebugAssertMsg(static_cast<const Collider3D *>(&A) == hit.A, std::string("Failure ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(static_cast<const Collider3D *>(&B) == hit.B, std::string("Failure ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0, 0.25, -0.1)).lengthSquared() < Constants::Epsilon, std::string("Failure ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0, 0.25, -0.05)).lengthSquared() < Constants::Epsilon, std::string("Failure ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitZ, std::string("Failure ") + __FUNCTION__ + std::string(": hit.Normal incorrect value"));

	DebugAssertMsg(B.CheckCollision(static_cast<const Collider3D &>(A), hit), std::string("Failure ") + __FUNCTION__ + std::string(": Invalid Return value"));
	DebugAssertMsg(static_cast<const Collider3D *>(&B) == hit.A, std::string("Failure ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(static_cast<const Collider3D *>(&A) == hit.B, std::string("Failure ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0, 0.25, -0.05)).lengthSquared() < Constants::Epsilon, std::string("Failure ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0, 0.25, -0.1)).lengthSquared() < Constants::Epsilon, std::string("Failure ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitZ, std::string("Failure ") + __FUNCTION__ + std::string(": hit.Normal incorrect value"));
}

void UnitTestCylinder_Cylinder_Top_PosX_Interpenetrating()
{
	AxisAlignedCylinderCollider3D A(Double3(0, 0, 0), 0.1, 0.5);
	AxisAlignedCylinderCollider3D B(Double3(0.15, 0.49, 0), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(static_cast<const Collider3D &>(B), hit), std::string("Failure ") + __FUNCTION__ + std::string(": Invalid Return value"));
	DebugAssertMsg(static_cast<const Collider3D *>(&A) == hit.A, std::string("Failure ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(static_cast<const Collider3D *>(&B) == hit.B, std::string("Failure ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.075, 0.5, 0)).lengthSquared() < Constants::Epsilon, std::string("Failure ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.075, 0.49, 0)).lengthSquared() < Constants::Epsilon, std::string("Failure ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitY, std::string("Failure ") + __FUNCTION__ + std::string(": hit.Normal incorrect value"));

	DebugAssertMsg(B.CheckCollision(static_cast<const Collider3D &>(A), hit), std::string("Failure ") + __FUNCTION__ + std::string(": Invalid Return value"));
	DebugAssertMsg(static_cast<const Collider3D *>(&B) == hit.A, std::string("Failure ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(static_cast<const Collider3D *>(&A) == hit.B, std::string("Failure ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.075, 0.49, 0)).lengthSquared() < Constants::Epsilon, std::string("Failure ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.075, 0.5, 0)).lengthSquared() < Constants::Epsilon, std::string("Failure ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitY, std::string("Failure ") + __FUNCTION__ + std::string(": hit.Normal incorrect value"));
}

void UnitTestCylinder_Cylinder_Top_PosZ_Interpenetrating()
{
	AxisAlignedCylinderCollider3D A(Double3(0, 0, 0), 0.1, 0.5);
	AxisAlignedCylinderCollider3D B(Double3(0, 0.49, 0.15), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(static_cast<const Collider3D &>(B), hit), std::string("Failure ") + __FUNCTION__ + std::string(": Invalid Return value"));
	DebugAssertMsg(static_cast<const Collider3D *>(&A) == hit.A, std::string("Failure ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(static_cast<const Collider3D *>(&B) == hit.B, std::string("Failure ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0, 0.5, 0.075)).lengthSquared() < Constants::Epsilon, std::string("Failure ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0, 0.49, 0.075)).lengthSquared() < Constants::Epsilon, std::string("Failure ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitY, std::string("Failure ") + __FUNCTION__ + std::string(": hit.Normal incorrect value"));

	DebugAssertMsg(B.CheckCollision(static_cast<const Collider3D &>(A), hit), std::string("Failure ") + __FUNCTION__ + std::string(": Invalid Return value"));
	DebugAssertMsg(static_cast<const Collider3D *>(&B) == hit.A, std::string("Failure ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(static_cast<const Collider3D *>(&A) == hit.B, std::string("Failure ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0, 0.49, 0.075)).lengthSquared() < Constants::Epsilon, std::string("Failure ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0, 0.5, 0.075)).lengthSquared() < Constants::Epsilon, std::string("Failure ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitY, std::string("Failure ") + __FUNCTION__ + std::string(": hit.Normal incorrect value"));
}

void UnitTestCylinder_Cylinder_Top_NegX_Interpenetrating()
{
	AxisAlignedCylinderCollider3D A(Double3(0, 0, 0), 0.1, 0.5);
	AxisAlignedCylinderCollider3D B(Double3(-0.15, 0.49, 0), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(static_cast<const Collider3D &>(B), hit), std::string("Failure ") + __FUNCTION__ + std::string(": Invalid Return value"));
	DebugAssertMsg(static_cast<const Collider3D *>(&A) == hit.A, std::string("Failure ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(static_cast<const Collider3D *>(&B) == hit.B, std::string("Failure ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(-0.075, 0.5, 0)).lengthSquared() < Constants::Epsilon, std::string("Failure ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(-0.075, 0.49, 0)).lengthSquared() < Constants::Epsilon, std::string("Failure ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitY, std::string("Failure ") + __FUNCTION__ + std::string(": hit.Normal incorrect value"));

	DebugAssertMsg(B.CheckCollision(static_cast<const Collider3D &>(A), hit), std::string("Failure ") + __FUNCTION__ + std::string(": Invalid Return value"));
	DebugAssertMsg(static_cast<const Collider3D *>(&B) == hit.A, std::string("Failure ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(static_cast<const Collider3D *>(&A) == hit.B, std::string("Failure ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(-0.075, 0.49, 0)).lengthSquared() < Constants::Epsilon, std::string("Failure ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(-0.075, 0.5, 0)).lengthSquared() < Constants::Epsilon, std::string("Failure ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitY, std::string("Failure ") + __FUNCTION__ + std::string(": hit.Normal incorrect value"));
}

void UnitTestCylinder_Cylinder_Top_NegZ_Interpenetrating()
{
	AxisAlignedCylinderCollider3D A(Double3(0, 0, 0), 0.1, 0.5);
	AxisAlignedCylinderCollider3D B(Double3(0, 0.49, -0.15), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(static_cast<const Collider3D &>(B), hit), std::string("Failure ") + __FUNCTION__ + std::string(": Invalid Return value"));
	DebugAssertMsg(static_cast<const Collider3D *>(&A) == hit.A, std::string("Failure ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(static_cast<const Collider3D *>(&B) == hit.B, std::string("Failure ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0, 0.5, -0.075)).lengthSquared() < Constants::Epsilon, std::string("Failure ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0, 0.49, -0.075)).lengthSquared() < Constants::Epsilon, std::string("Failure ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitY, std::string("Failure ") + __FUNCTION__ + std::string(": hit.Normal incorrect value"));

	DebugAssertMsg(B.CheckCollision(static_cast<const Collider3D &>(A), hit), std::string("Failure ") + __FUNCTION__ + std::string(": Invalid Return value"));
	DebugAssertMsg(static_cast<const Collider3D *>(&B) == hit.A, std::string("Failure ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(static_cast<const Collider3D *>(&A) == hit.B, std::string("Failure ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0, 0.49, -0.075)).lengthSquared() < Constants::Epsilon, std::string("Failure ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0, 0.5, -0.075)).lengthSquared() < Constants::Epsilon, std::string("Failure ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitY, std::string("Failure ") + __FUNCTION__ + std::string(": hit.Normal incorrect value"));
}

void UnitTestCylinder_Cylinder_Bottom_PosX_Interpenetrating()
{
	AxisAlignedCylinderCollider3D A(Double3(0, 0, 0), 0.1, 0.5);
	AxisAlignedCylinderCollider3D B(Double3(0.15, -0.49, 0), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(static_cast<const Collider3D &>(B), hit), std::string("Failure ") + __FUNCTION__ + std::string(": Invalid Return value"));
	DebugAssertMsg(static_cast<const Collider3D *>(&A) == hit.A, std::string("Failure ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(static_cast<const Collider3D *>(&B) == hit.B, std::string("Failure ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.075, 0, 0)).lengthSquared() < Constants::Epsilon, std::string("Failure ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.075, 0.01, 0)).lengthSquared() < Constants::Epsilon, std::string("Failure ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitY, std::string("Failure ") + __FUNCTION__ + std::string(": hit.Normal incorrect value"));

	DebugAssertMsg(B.CheckCollision(static_cast<const Collider3D &>(A), hit), std::string("Failure ") + __FUNCTION__ + std::string(": Invalid Return value"));
	DebugAssertMsg(static_cast<const Collider3D *>(&B) == hit.A, std::string("Failure ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(static_cast<const Collider3D *>(&A) == hit.B, std::string("Failure ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0.075, 0.01, 0)).lengthSquared() < Constants::Epsilon, std::string("Failure ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0.075, 0, 0)).lengthSquared() < Constants::Epsilon, std::string("Failure ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitY, std::string("Failure ") + __FUNCTION__ + std::string(": hit.Normal incorrect value"));
}

void UnitTestCylinder_Cylinder_Bottom_PosZ_Interpenetrating()
{
	AxisAlignedCylinderCollider3D A(Double3(0, 0, 0), 0.1, 0.5);
	AxisAlignedCylinderCollider3D B(Double3(0, -0.49, 0.15), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(static_cast<const Collider3D &>(B), hit), std::string("Failure ") + __FUNCTION__ + std::string(": Invalid Return value"));
	DebugAssertMsg(static_cast<const Collider3D *>(&A) == hit.A, std::string("Failure ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(static_cast<const Collider3D *>(&B) == hit.B, std::string("Failure ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0, 0, 0.075)).lengthSquared() < Constants::Epsilon, std::string("Failure ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0, 0.01, 0.075)).lengthSquared() < Constants::Epsilon, std::string("Failure ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitY, std::string("Failure ") + __FUNCTION__ + std::string(": hit.Normal incorrect value"));

	DebugAssertMsg(B.CheckCollision(static_cast<const Collider3D &>(A), hit), std::string("Failure ") + __FUNCTION__ + std::string(": Invalid Return value"));
	DebugAssertMsg(static_cast<const Collider3D *>(&B) == hit.A, std::string("Failure ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(static_cast<const Collider3D *>(&A) == hit.B, std::string("Failure ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0, 0.01, 0.075)).lengthSquared() < Constants::Epsilon, std::string("Failure ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0, 0, 0.075)).lengthSquared() < Constants::Epsilon, std::string("Failure ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitY, std::string("Failure ") + __FUNCTION__ + std::string(": hit.Normal incorrect value"));
}

void UnitTestCylinder_Cylinder_Bottom_NegX_Interpenetrating()
{
	AxisAlignedCylinderCollider3D A(Double3(0, 0, 0), 0.1, 0.5);
	AxisAlignedCylinderCollider3D B(Double3(-0.15, -0.49, 0), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(static_cast<const Collider3D &>(B), hit), std::string("Failure ") + __FUNCTION__ + std::string(": Invalid Return value"));
	DebugAssertMsg(static_cast<const Collider3D *>(&A) == hit.A, std::string("Failure ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(static_cast<const Collider3D *>(&B) == hit.B, std::string("Failure ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(-0.075, 0, 0)).lengthSquared() < Constants::Epsilon, std::string("Failure ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(-0.075, 0.01, 0)).lengthSquared() < Constants::Epsilon, std::string("Failure ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitY, std::string("Failure ") + __FUNCTION__ + std::string(": hit.Normal incorrect value"));

	DebugAssertMsg(B.CheckCollision(static_cast<const Collider3D &>(A), hit), std::string("Failure ") + __FUNCTION__ + std::string(": Invalid Return value"));
	DebugAssertMsg(static_cast<const Collider3D *>(&B) == hit.A, std::string("Failure ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(static_cast<const Collider3D *>(&A) == hit.B, std::string("Failure ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(-0.075, 0.01, 0)).lengthSquared() < Constants::Epsilon, std::string("Failure ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(-0.075, 0, 0)).lengthSquared() < Constants::Epsilon, std::string("Failure ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitY, std::string("Failure ") + __FUNCTION__ + std::string(": hit.Normal incorrect value"));
}

void UnitTestCylinder_Cylinder_Bottom_NegZ_Interpenetrating()
{
	AxisAlignedCylinderCollider3D A(Double3(0, 0, 0), 0.1, 0.5);
	AxisAlignedCylinderCollider3D B(Double3(0, -0.49, -0.15), 0.1, 0.5);
	Collider3D::ColliderHit hit(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);

	DebugAssertMsg(A.CheckCollision(static_cast<const Collider3D &>(B), hit), std::string("Failure ") + __FUNCTION__ + std::string(": Invalid Return value"));
	DebugAssertMsg(static_cast<const Collider3D *>(&A) == hit.A, std::string("Failure ") + __FUNCTION__ + std::string(": hit.A != A"));
	DebugAssertMsg(static_cast<const Collider3D *>(&B) == hit.B, std::string("Failure ") + __FUNCTION__ + std::string(": hit.B != B"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0, 0, -0.075)).lengthSquared() < Constants::Epsilon, std::string("Failure ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0, 0.01, -0.075)).lengthSquared() < Constants::Epsilon, std::string("Failure ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB incorrect value"));
	DebugAssertMsg(hit.Normal == Double3::UnitY, std::string("Failure ") + __FUNCTION__ + std::string(": hit.Normal incorrect value"));

	DebugAssertMsg(B.CheckCollision(static_cast<const Collider3D &>(A), hit), std::string("Failure ") + __FUNCTION__ + std::string(": Invalid Return value"));
	DebugAssertMsg(static_cast<const Collider3D *>(&B) == hit.A, std::string("Failure ") + __FUNCTION__ + std::string(": hit.A != B"));
	DebugAssertMsg(static_cast<const Collider3D *>(&A) == hit.B, std::string("Failure ") + __FUNCTION__ + std::string(": hit.B != A"));
	DebugAssertMsg((hit.PointOfImpactOnA - Double3(0, 0.01, -0.075)).lengthSquared() < Constants::Epsilon, std::string("Failure ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnA incorrect value"));
	DebugAssertMsg((hit.PointOfImpactOnB - Double3(0, 0, -0.075)).lengthSquared() < Constants::Epsilon, std::string("Failure ") + __FUNCTION__ + std::string(": hit.PointOfImpactOnB incorrect value"));
	DebugAssertMsg(hit.Normal == -Double3::UnitY, std::string("Failure ") + __FUNCTION__ + std::string(": hit.Normal incorrect value"));
}

#pragma endregion Unit Tests - Axis Aligned Cylinder -> Axis Aligned Cylinder

#pragma region Unit Tests - Axis Aligned Cylinder -> Quad

#pragma region Unit Tests - Cylinder near center of Quad

// Check for collision when the cylinder is too far in front of the quad
void UnitTestCylinder_Quad_NoCollision_NormalPositive()
{
	QuadCollider3D A(Double3(0.5, 0.5, 0.5), Double3(-1, 0, -1), sqrt(2), 1);
	AxisAlignedCylinderCollider3D B(Double3(0, 0, 0), 0.1, 0.5);

	Collider3D::ColliderHit hitInfo(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);
	DebugAssertMsg(!A.CheckCollision(static_cast<const Collider3D&>(B), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(!B.CheckCollision(static_cast<const Collider3D&>(A), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
}

// Check for collision against the front of the quad (normal facing the cylinder)
void UnitTestCylinder_Quad_Touching_NormalPositive()
{
	QuadCollider3D A(Double3(0.5, 0.5, 0.5), Double3(-1, 0, -1), sqrt(2), 1);
	AxisAlignedCylinderCollider3D B(Double3(0.5, 0, 0.5) + (A.Normal * 0.1), 0.1, 0.5);

	Collider3D::ColliderHit hitInfo(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);
	DebugAssertMsg(A.CheckCollision(static_cast<const Collider3D&>(B), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(hitInfo.A == static_cast<const Collider3D*>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for hitInfo.A"));
	DebugAssertMsg(hitInfo.B == static_cast<const Collider3D*>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for hitInfo.B"));
	DebugAssertMsg(CloseEnough(hitInfo.Normal, -A.Normal), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for hitInfo.Normal"));
	DebugAssertMsg(hitInfo.PointOfImpactOnA == Double3(0.5, 0.25, 0.5), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for hitInfo.PointOfImpactOnA"));
	DebugAssertMsg(hitInfo.PointOfImpactOnB == Double3(0.5, 0.25, 0.5), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for hitInfo.PointOfImpactOnB"));
	
	DebugAssertMsg(B.CheckCollision(static_cast<const Collider3D&>(A), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(hitInfo.A == static_cast<const Collider3D*>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for hitInfo.A"));
	DebugAssertMsg(hitInfo.B == static_cast<const Collider3D*>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for hitInfo.B"));
	DebugAssertMsg(CloseEnough(hitInfo.Normal, A.Normal), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for hitInfo.Normal"));
	DebugAssertMsg(hitInfo.PointOfImpactOnA == Double3(0.5, 0.25, 0.5), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for hitInfo.PointOfImpactOnA"));
	DebugAssertMsg(hitInfo.PointOfImpactOnB == Double3(0.5, 0.25, 0.5), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for hitInfo.PointOfImpactOnB"));
}

// Check for collision against the front of the quad (normal facing the cylinder)
void UnitTestCylinder_Quad_Interpenetrating_NormalPositive()
{
	QuadCollider3D A(Double3(0.5, 0.5, 0.5), Double3(-1, 0, -1), sqrt(2), 1);
	AxisAlignedCylinderCollider3D B(Double3(0.5, 0, 0.5) + (A.Normal * 0.05), 0.1, 0.5);

	Collider3D::ColliderHit hitInfo(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);
	DebugAssertMsg(A.CheckCollision(static_cast<const Collider3D&>(B), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(hitInfo.A == static_cast<const Collider3D*>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for hitInfo.A"));
	DebugAssertMsg(hitInfo.B == static_cast<const Collider3D*>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for hitInfo.B"));
	DebugAssertMsg(CloseEnough(hitInfo.Normal, -A.Normal), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for hitInfo.Normal"));
	DebugAssertMsg(hitInfo.PointOfImpactOnA == Double3(0.5, 0.25, 0.5), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for hitInfo.PointOfImpactOnA"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnB, Double3(0.5, 0.25, 0.5) - (A.Normal * 0.05)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for hitInfo.PointOfImpactOnB"));

	DebugAssertMsg(B.CheckCollision(static_cast<const Collider3D&>(A), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(hitInfo.A == static_cast<const Collider3D*>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for hitInfo.A"));
	DebugAssertMsg(hitInfo.B == static_cast<const Collider3D*>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for hitInfo.B"));
	DebugAssertMsg(CloseEnough(hitInfo.Normal, A.Normal), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for hitInfo.Normal"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnA, Double3(0.5, 0.25, 0.5) - (A.Normal * 0.05)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for hitInfo.PointOfImpactOnA"));
	DebugAssertMsg(hitInfo.PointOfImpactOnB == Double3(0.5, 0.25, 0.5), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for hitInfo.PointOfImpactOnB"));
}

// Check for collision against the back of the quad (normal facing away from the cylinder)
void UnitTestCylinder_Quad_Interpenetrating_NormalNegative()
{
	QuadCollider3D A(Double3(0.5, 0.5, 0.5), Double3(-1, 0, -1), sqrt(2), 1);
	AxisAlignedCylinderCollider3D B(Double3(0.5, 0, 0.5) - (A.Normal * 0.05), 0.1, 0.5);

	Collider3D::ColliderHit hitInfo(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);
	DebugAssertMsg(A.CheckCollision(static_cast<const Collider3D&>(B), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(hitInfo.A == static_cast<const Collider3D*>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for hitInfo.A"));
	DebugAssertMsg(hitInfo.B == static_cast<const Collider3D*>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for hitInfo.B"));
	DebugAssertMsg(CloseEnough(hitInfo.Normal, A.Normal), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for hitInfo.Normal"));
	DebugAssertMsg(hitInfo.PointOfImpactOnA == Double3(0.5, 0.25, 0.5), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for hitInfo.PointOfImpactOnA"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnB, Double3(0.5, 0.25, 0.5) + (A.Normal * 0.05)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for hitInfo.PointOfImpactOnB"));
	
	DebugAssertMsg(B.CheckCollision(static_cast<const Collider3D&>(A), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(hitInfo.A == static_cast<const Collider3D*>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for hitInfo.A"));
	DebugAssertMsg(hitInfo.B == static_cast<const Collider3D*>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for hitInfo.B"));
	DebugAssertMsg(CloseEnough(hitInfo.Normal, -A.Normal), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for hitInfo.Normal"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnA, Double3(0.5, 0.25, 0.5) + (A.Normal * 0.05)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for hitInfo.PointOfImpactOnA"));
	DebugAssertMsg(hitInfo.PointOfImpactOnB == Double3(0.5, 0.25, 0.5), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for hitInfo.PointOfImpactOnB"));
}

// Check for collision against the back of the quad (normal facing away from the cylinder)
void UnitTestCylinder_Quad_Touching_NormalNegative()
{
	QuadCollider3D A(Double3(0.5, 0.5, 0.5), Double3(-1, 0, -1), sqrt(2), 1);
	AxisAlignedCylinderCollider3D B(Double3(0.5, 0, 0.5) - (A.Normal * 0.1), 0.1, 0.5);

	Collider3D::ColliderHit hitInfo(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);
	DebugAssertMsg(A.CheckCollision(static_cast<const Collider3D&>(B), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(hitInfo.A == static_cast<const Collider3D*>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for hitInfo.A"));
	DebugAssertMsg(hitInfo.B == static_cast<const Collider3D*>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for hitInfo.B"));
	DebugAssertMsg(CloseEnough(hitInfo.Normal, A.Normal), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for hitInfo.Normal"));
	DebugAssertMsg(hitInfo.PointOfImpactOnA == Double3(0.5, 0.25, 0.5), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for hitInfo.PointOfImpactOnA"));
	DebugAssertMsg(hitInfo.PointOfImpactOnB == Double3(0.5, 0.25, 0.5), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for hitInfo.PointOfImpactOnB"));

	DebugAssertMsg(B.CheckCollision(static_cast<const Collider3D&>(A), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(hitInfo.A == static_cast<const Collider3D*>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for hitInfo.A"));
	DebugAssertMsg(hitInfo.B == static_cast<const Collider3D*>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for hitInfo.B"));
	DebugAssertMsg(CloseEnough(hitInfo.Normal, -A.Normal), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for hitInfo.Normal"));
	DebugAssertMsg(hitInfo.PointOfImpactOnA == Double3(0.5, 0.25, 0.5), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for hitInfo.PointOfImpactOnA"));
	DebugAssertMsg(hitInfo.PointOfImpactOnB == Double3(0.5, 0.25, 0.5), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for hitInfo.PointOfImpactOnB"));
}

// Check for collision when the cylinder is too far behind the quad
void UnitTestCylinder_Quad_NoCollision_NormalNegative()
{
	QuadCollider3D A(Double3(0.5, 0.5, 0.5), Double3(-1, 0, -1), sqrt(2), 1);
	AxisAlignedCylinderCollider3D B(Double3(1, 0, 1), 0.1, 0.5);

	Collider3D::ColliderHit hitInfo(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);
	DebugAssertMsg(!A.CheckCollision(static_cast<const Collider3D&>(B), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(!B.CheckCollision(static_cast<const Collider3D&>(A), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
}

#pragma endregion Unit Tests - Cylinder near center of Quad

#pragma region Unit Tests - Cylinder near Sides of Quad

void UnitTestCylinder_Quad_NoCollision_NormalPositive_PositiveXEdge()
{
	QuadCollider3D A(Double3(0.5, 0.5, 0.5), Double3(-1, 0, -1), sqrt(2), 1);
	AxisAlignedCylinderCollider3D B(Double3(1.11, 0, 0), 0.1, 0.5);

	Collider3D::ColliderHit hitInfo(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);
	DebugAssertMsg(!A.CheckCollision(static_cast<const Collider3D&>(B), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(!B.CheckCollision(static_cast<const Collider3D&>(A), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
}

void UnitTestCylinder_Quad_Touching_NormalPositive_PositiveXEdge()
{
	QuadCollider3D A(Double3(0.5, 0.5, 0.5), Double3(-1, 0, -1), sqrt(2), 1);
	AxisAlignedCylinderCollider3D B(Double3(1.099, 0, 0), 0.1, 0.5);

	Collider3D::ColliderHit hitInfo(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);
	DebugAssertMsg(A.CheckCollision(static_cast<const Collider3D&>(B), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(hitInfo.A == static_cast<const Collider3D*>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for A"));
	DebugAssertMsg(hitInfo.B == static_cast<const Collider3D*>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for B"));
	DebugAssertMsg(CloseEnough(hitInfo.Normal, Double3(-1, 0, 0)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect Normal"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnA, Double3(1, 0.25, 0)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnB, Double3(1, 0.25, 0)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));

	DebugAssertMsg(B.CheckCollision(static_cast<const Collider3D&>(A), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(hitInfo.A == static_cast<const Collider3D*>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for A"));
	DebugAssertMsg(hitInfo.B == static_cast<const Collider3D*>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for B"));
	DebugAssertMsg(CloseEnough(hitInfo.Normal, Double3(1, 0, 0)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect Normal"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnA, Double3(1, 0.25, 0)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnB, Double3(1, 0.25, 0)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));
}

void UnitTestCylinder_Quad_Interpenetrating_NormalPositive_PositiveXEdge()
{
	QuadCollider3D A(Double3(0.5, 0.5, 0.5), Double3(-1, 0, -1), sqrt(2), 1);
	AxisAlignedCylinderCollider3D B(Double3(1.075, 0, 0), 0.1, 0.5);

	Collider3D::ColliderHit hitInfo(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);
	DebugAssertMsg(A.CheckCollision(static_cast<const Collider3D&>(B), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(hitInfo.A == static_cast<const Collider3D*>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for A"));
	DebugAssertMsg(hitInfo.B == static_cast<const Collider3D*>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for B"));
	DebugAssertMsg(CloseEnough(hitInfo.Normal, Double3(-1, 0, 0)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect Normal"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnA, Double3(1, 0.25, 0)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnB, Double3(0.975, 0.25, 0)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));

	DebugAssertMsg(B.CheckCollision(static_cast<const Collider3D&>(A), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(hitInfo.A == static_cast<const Collider3D*>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for A"));
	DebugAssertMsg(hitInfo.B == static_cast<const Collider3D*>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for B"));
	DebugAssertMsg(CloseEnough(hitInfo.Normal, Double3(1, 0, 0)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect Normal"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnA, Double3(0.975, 0.25, 0)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnB, Double3(1, 0.25, 0)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));
}

void UnitTestCylinder_Quad_NoCollision_NormalNegative_PositiveXEdge()
{
	QuadCollider3D A(Double3(0.5, 0.5, 0.5), Double3(-1, 0, -1), sqrt(2), 1);
	AxisAlignedCylinderCollider3D B(Double3(1, 0, -0.11), 0.1, 0.5);

	Collider3D::ColliderHit hitInfo(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);
	DebugAssertMsg(!A.CheckCollision(static_cast<const Collider3D&>(B), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(!B.CheckCollision(static_cast<const Collider3D&>(A), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
}

void UnitTestCylinder_Quad_Touching_NormalNegative_PositiveXEdge()
{
	QuadCollider3D A(Double3(0.5, 0.5, 0.5), Double3(-1, 0, -1), sqrt(2), 1);
	AxisAlignedCylinderCollider3D B(Double3(1, 0, -0.099), 0.1, 0.5);

	Collider3D::ColliderHit hitInfo(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);
	DebugAssertMsg(A.CheckCollision(static_cast<const Collider3D&>(B), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(hitInfo.A == static_cast<const Collider3D*>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for A"));
	DebugAssertMsg(hitInfo.B == static_cast<const Collider3D*>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for B"));
	DebugAssertMsg(CloseEnough(hitInfo.Normal, Double3(0, 0, 1)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect Normal"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnA, Double3(1, 0.25, 0)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnB, Double3(1, 0.25, 0)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));

	DebugAssertMsg(B.CheckCollision(static_cast<const Collider3D&>(A), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(hitInfo.A == static_cast<const Collider3D*>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for A"));
	DebugAssertMsg(hitInfo.B == static_cast<const Collider3D*>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for B"));
	DebugAssertMsg(CloseEnough(hitInfo.Normal, Double3(0, 0, -1)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect Normal"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnA, Double3(1, 0.25, 0)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnB, Double3(1, 0.25, 0)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));
}

void UnitTestCylinder_Quad_Interpenetrating_NormalNegative_PositiveXEdge()
{
	QuadCollider3D A(Double3(0.5, 0.5, 0.5), Double3(-1, 0, -1), sqrt(2), 1);
	AxisAlignedCylinderCollider3D B(Double3(1, 0, -0.075), 0.1, 0.5);

	Collider3D::ColliderHit hitInfo(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);
	DebugAssertMsg(A.CheckCollision(static_cast<const Collider3D&>(B), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(hitInfo.A == static_cast<const Collider3D*>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for A"));
	DebugAssertMsg(hitInfo.B == static_cast<const Collider3D*>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for B"));
	DebugAssertMsg(CloseEnough(hitInfo.Normal, Double3(0, 0, 1)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect Normal"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnA, Double3(1, 0.25, 0)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnB, Double3(1, 0.25, 0.025)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));

	DebugAssertMsg(B.CheckCollision(static_cast<const Collider3D&>(A), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(hitInfo.A == static_cast<const Collider3D*>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for A"));
	DebugAssertMsg(hitInfo.B == static_cast<const Collider3D*>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for B"));
	DebugAssertMsg(CloseEnough(hitInfo.Normal, Double3(0, 0, -1)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect Normal"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnA, Double3(1, 0.25, 0.025)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnB, Double3(1, 0.25, 0)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));
}

void UnitTestCylinder_Quad_NoCollision_NormalPositive_NegativeXEdge()
{
	QuadCollider3D A(Double3(0.5, 0.5, 0.5), Double3(-1, 0, -1), sqrt(2), 1);
	AxisAlignedCylinderCollider3D B(Double3(0, 0, 1.11), 0.1, 0.5);

	Collider3D::ColliderHit hitInfo(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);
	DebugAssertMsg(!A.CheckCollision(static_cast<const Collider3D&>(B), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(!B.CheckCollision(static_cast<const Collider3D&>(A), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
}

void UnitTestCylinder_Quad_Touching_NormalPositive_NegativeXEdge()
{
	QuadCollider3D A(Double3(0.5, 0.5, 0.5), Double3(-1, 0, -1), sqrt(2), 1);
	AxisAlignedCylinderCollider3D B(Double3(0, 0, 1.099), 0.1, 0.5);

	Collider3D::ColliderHit hitInfo(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);
	DebugAssertMsg(A.CheckCollision(static_cast<const Collider3D&>(B), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(hitInfo.A == static_cast<const Collider3D*>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for A"));
	DebugAssertMsg(hitInfo.B == static_cast<const Collider3D*>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for B"));
	DebugAssertMsg(CloseEnough(hitInfo.Normal, Double3(0, 0, -1)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect Normal"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnA, Double3(0, 0.25, 1)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnB, Double3(0, 0.25, 1)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));

	DebugAssertMsg(B.CheckCollision(static_cast<const Collider3D&>(A), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(hitInfo.A == static_cast<const Collider3D*>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for A"));
	DebugAssertMsg(hitInfo.B == static_cast<const Collider3D*>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for B"));
	DebugAssertMsg(CloseEnough(hitInfo.Normal, Double3(0, 0, 1)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect Normal"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnA, Double3(0, 0.25, 1)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnB, Double3(0, 0.25, 1)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));
}

void UnitTestCylinder_Quad_Interpenetrating_NormalPositive_NegativeXEdge()
{
	QuadCollider3D A(Double3(0.5, 0.5, 0.5), Double3(-1, 0, -1), sqrt(2), 1);
	AxisAlignedCylinderCollider3D B(Double3(0, 0, 1.075), 0.1, 0.5);

	Collider3D::ColliderHit hitInfo(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);
	DebugAssertMsg(A.CheckCollision(static_cast<const Collider3D&>(B), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(hitInfo.A == static_cast<const Collider3D*>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for A"));
	DebugAssertMsg(hitInfo.B == static_cast<const Collider3D*>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for B"));
	DebugAssertMsg(CloseEnough(hitInfo.Normal, Double3(0, 0, -1)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect Normal"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnA, Double3(0, 0.25, 1)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnB, Double3(0, 0.25, 0.975)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));

	DebugAssertMsg(B.CheckCollision(static_cast<const Collider3D&>(A), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(hitInfo.A == static_cast<const Collider3D*>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for A"));
	DebugAssertMsg(hitInfo.B == static_cast<const Collider3D*>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for B"));
	DebugAssertMsg(CloseEnough(hitInfo.Normal, Double3(0, 0, 1)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect Normal"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnA, Double3(0, 0.25, 0.975)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnB, Double3(0, 0.25, 1)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));
}

void UnitTestCylinder_Quad_NoCollision_NormalNegative_NegativeXEdge()
{
	QuadCollider3D A(Double3(0.5, 0.5, 0.5), Double3(-1, 0, -1), sqrt(2), 1);
	AxisAlignedCylinderCollider3D B(Double3(-0.11, 0, 1), 0.1, 0.5);

	Collider3D::ColliderHit hitInfo(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);
	DebugAssertMsg(!A.CheckCollision(static_cast<const Collider3D&>(B), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(!B.CheckCollision(static_cast<const Collider3D&>(A), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
}

void UnitTestCylinder_Quad_Touching_NormalNegative_NegativeXEdge()
{
	QuadCollider3D A(Double3(0.5, 0.5, 0.5), Double3(-1, 0, -1), sqrt(2), 1);
	AxisAlignedCylinderCollider3D B(Double3(-0.099, 0, 1), 0.1, 0.5);

	Collider3D::ColliderHit hitInfo(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);
	DebugAssertMsg(A.CheckCollision(static_cast<const Collider3D&>(B), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(hitInfo.A == static_cast<const Collider3D*>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for A"));
	DebugAssertMsg(hitInfo.B == static_cast<const Collider3D*>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for B"));
	DebugAssertMsg(CloseEnough(hitInfo.Normal, Double3(1, 0, 0)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect Normal"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnA, Double3(0, 0.25, 1)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnB, Double3(0, 0.25, 1)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));

	DebugAssertMsg(B.CheckCollision(static_cast<const Collider3D&>(A), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(hitInfo.A == static_cast<const Collider3D*>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for A"));
	DebugAssertMsg(hitInfo.B == static_cast<const Collider3D*>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for B"));
	DebugAssertMsg(CloseEnough(hitInfo.Normal, Double3(-1, 0, 0)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect Normal"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnA, Double3(0, 0.25, 1)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnB, Double3(0, 0.25, 1)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));
}

void UnitTestCylinder_Quad_Interpenetrating_NormalNegative_NegativeXEdge()
{
	QuadCollider3D A(Double3(0.5, 0.5, 0.5), Double3(-1, 0, -1), sqrt(2), 1);
	AxisAlignedCylinderCollider3D B(Double3(-0.075, 0, 1), 0.1, 0.5);

	Collider3D::ColliderHit hitInfo(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);
	DebugAssertMsg(A.CheckCollision(static_cast<const Collider3D&>(B), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(hitInfo.A == static_cast<const Collider3D*>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for A"));
	DebugAssertMsg(hitInfo.B == static_cast<const Collider3D*>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for B"));
	DebugAssertMsg(CloseEnough(hitInfo.Normal, Double3(1, 0, 0)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect Normal"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnA, Double3(0, 0.25, 1)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnB, Double3(0.025, 0.25, 1)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));

	DebugAssertMsg(B.CheckCollision(static_cast<const Collider3D&>(A), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(hitInfo.A == static_cast<const Collider3D*>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for A"));
	DebugAssertMsg(hitInfo.B == static_cast<const Collider3D*>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for B"));
	DebugAssertMsg(CloseEnough(hitInfo.Normal, Double3(-1, 0, 0)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect Normal"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnA, Double3(0.025, 0.25, 1)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnB, Double3(0, 0.25, 1)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));
}

#pragma endregion Unit Tests - Cylinder near Sides of Quad

#pragma region Unit Tests - Cylinder near Top/Bottom of Quad

void UnitTestCylinder_Quad_NoCollision_NormalPositive_PositiveYEdge()
{
	QuadCollider3D A(Double3(0.5, 0.5, 0.5), Double3(-1, 0, -1), sqrt(2), 1);
	AxisAlignedCylinderCollider3D B(Double3(0.495, 1.01, 0.495), 0.1, 0.5);

	Collider3D::ColliderHit hitInfo(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);
	DebugAssertMsg(!A.CheckCollision(static_cast<const Collider3D&>(B), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(!B.CheckCollision(static_cast<const Collider3D&>(A), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
}

void UnitTestCylinder_Quad_Touching_NormalPositive_PositiveYEdge()
{
	QuadCollider3D A(Double3(0.5, 0.5, 0.5), Double3(-1, 0, -1), sqrt(2), 1);
	AxisAlignedCylinderCollider3D B(Double3(0.495, 1, 0.495), 0.1, 0.5);

	Collider3D::ColliderHit hitInfo(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);
	DebugAssertMsg(A.CheckCollision(static_cast<const Collider3D&>(B), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(hitInfo.A == static_cast<const Collider3D*>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for A"));
	DebugAssertMsg(hitInfo.B == static_cast<const Collider3D*>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for B"));
	DebugAssertMsg(CloseEnough(hitInfo.Normal, Double3(1, 0, 1).normalized()), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect Normal"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnA, Double3(0.5, 1, 0.5)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnB, Double3(0.495, 1, 0.495) + (Double3(1, 0, 1).normalized() * 0.1)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));

	DebugAssertMsg(B.CheckCollision(static_cast<const Collider3D&>(A), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(hitInfo.A == static_cast<const Collider3D*>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for A"));
	DebugAssertMsg(hitInfo.B == static_cast<const Collider3D*>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for B"));
	DebugAssertMsg(CloseEnough(hitInfo.Normal, Double3(-1, 0, -1).normalized()), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect Normal"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnA, Double3(0.495, 1, 0.495) + (Double3(1, 0, 1).normalized() * 0.1)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnB, Double3(0.5, 1, 0.5)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));
}

void UnitTestCylinder_Quad_Interpenetrating_NormalPositive_PositiveYEdge()
{
	QuadCollider3D A(Double3(0.5, 0.5, 0.5), Double3(-1, 0, -1), sqrt(2), 1);
	AxisAlignedCylinderCollider3D B(Double3(0.495, 0.99, 0.495), 0.1, 0.5);

	Collider3D::ColliderHit hitInfo(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);
	DebugAssertMsg(A.CheckCollision(static_cast<const Collider3D&>(B), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(hitInfo.A == static_cast<const Collider3D*>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for A"));
	DebugAssertMsg(hitInfo.B == static_cast<const Collider3D*>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for B"));
	DebugAssertMsg(CloseEnough(hitInfo.Normal, Double3(1, 0, 1).normalized()), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect Normal"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnA, Double3(0.5, 1, 0.5)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnB, Double3(0.495, 1, 0.495) + (Double3(1, 0, 1).normalized() * 0.1)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));

	DebugAssertMsg(B.CheckCollision(static_cast<const Collider3D&>(A), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(hitInfo.A == static_cast<const Collider3D*>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for A"));
	DebugAssertMsg(hitInfo.B == static_cast<const Collider3D*>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for B"));
	DebugAssertMsg(CloseEnough(hitInfo.Normal, Double3(-1, 0, -1).normalized()), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect Normal"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnA, Double3(0.495, 1, 0.495) + (Double3(1, 0, 1).normalized() * 0.1)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnB, Double3(0.5, 1, 0.5)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));
}

void UnitTestCylinder_Quad_NoCollision_NormalNegative_PositiveYEdge()
{
	QuadCollider3D A(Double3(0.5, 0.5, 0.5), Double3(-1, 0, -1), sqrt(2), 1);
	AxisAlignedCylinderCollider3D B(Double3(0.505, 1.01, 0.505), 0.1, 0.5);

	Collider3D::ColliderHit hitInfo(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);
	DebugAssertMsg(!A.CheckCollision(static_cast<const Collider3D&>(B), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(!B.CheckCollision(static_cast<const Collider3D&>(A), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
}

void UnitTestCylinder_Quad_Touching_NormalNegative_PositiveYEdge()
{
	QuadCollider3D A(Double3(0.5, 0.5, 0.5), Double3(-1, 0, -1), sqrt(2), 1);
	AxisAlignedCylinderCollider3D B(Double3(0.505, 1, 0.505), 0.1, 0.5);

	Collider3D::ColliderHit hitInfo(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);
	DebugAssertMsg(A.CheckCollision(static_cast<const Collider3D&>(B), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(hitInfo.A == static_cast<const Collider3D*>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for A"));
	DebugAssertMsg(hitInfo.B == static_cast<const Collider3D*>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for B"));
	DebugAssertMsg(CloseEnough(hitInfo.Normal, Double3(-1, 0, -1).normalized()), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect Normal"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnA, Double3(0.5, 1, 0.5)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnB, Double3(0.505, 1, 0.505) - (Double3(1, 0, 1).normalized() * 0.1)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));

	DebugAssertMsg(B.CheckCollision(static_cast<const Collider3D&>(A), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(hitInfo.A == static_cast<const Collider3D*>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for A"));
	DebugAssertMsg(hitInfo.B == static_cast<const Collider3D*>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for B"));
	DebugAssertMsg(CloseEnough(hitInfo.Normal, Double3(1, 0, 1).normalized()), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect Normal"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnA, Double3(0.505, 1, 0.505) - (Double3(1, 0, 1).normalized() * 0.1)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnB, Double3(0.5, 1, 0.5)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));
}

void UnitTestCylinder_Quad_Interpenetrating_NormalNegative_PositiveYEdge()
{
	QuadCollider3D A(Double3(0.5, 0.5, 0.5), Double3(-1, 0, -1), sqrt(2), 1);
	AxisAlignedCylinderCollider3D B(Double3(0.505, 0.99, 0.505), 0.1, 0.5);

	Collider3D::ColliderHit hitInfo(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);
	DebugAssertMsg(A.CheckCollision(static_cast<const Collider3D&>(B), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(hitInfo.A == static_cast<const Collider3D*>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for A"));
	DebugAssertMsg(hitInfo.B == static_cast<const Collider3D*>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for B"));
	DebugAssertMsg(CloseEnough(hitInfo.Normal, Double3(-1, 0, -1).normalized()), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect Normal"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnA, Double3(0.5, 1, 0.5)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnB, Double3(0.505, 1, 0.505) - (Double3(1, 0, 1).normalized() * 0.1)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));

	DebugAssertMsg(B.CheckCollision(static_cast<const Collider3D&>(A), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(hitInfo.A == static_cast<const Collider3D*>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for A"));
	DebugAssertMsg(hitInfo.B == static_cast<const Collider3D*>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for B"));
	DebugAssertMsg(CloseEnough(hitInfo.Normal, Double3(1, 0, 1).normalized()), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect Normal"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnA, Double3(0.505, 1, 0.505) - (Double3(1, 0, 1).normalized() * 0.1)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnB, Double3(0.5, 1, 0.5)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));
}

void UnitTestCylinder_Quad_NoCollision_NormalPositive_NegativeYEdge()
{
	QuadCollider3D A(Double3(0.5, 0.5, 0.5), Double3(-1, 0, -1), sqrt(2), 1);
	AxisAlignedCylinderCollider3D B(Double3(0.495, -0.51, 0.495), 0.1, 0.5);

	Collider3D::ColliderHit hitInfo(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);
	DebugAssertMsg(!A.CheckCollision(static_cast<const Collider3D&>(B), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(!B.CheckCollision(static_cast<const Collider3D&>(A), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
}

void UnitTestCylinder_Quad_Touching_NormalPositive_NegativeYEdge()
{
	QuadCollider3D A(Double3(0.5, 0.5, 0.5), Double3(-1, 0, -1), sqrt(2), 1);
	AxisAlignedCylinderCollider3D B(Double3(0.495, -0.5, 0.495), 0.1, 0.5);

	Collider3D::ColliderHit hitInfo(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);
	DebugAssertMsg(A.CheckCollision(static_cast<const Collider3D&>(B), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(hitInfo.A == static_cast<const Collider3D*>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for A"));
	DebugAssertMsg(hitInfo.B == static_cast<const Collider3D*>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for B"));
	DebugAssertMsg(CloseEnough(hitInfo.Normal, Double3(1, 0, 1).normalized()), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect Normal"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnA, Double3(0.5, 0, 0.5)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnB, Double3(0.495, 0, 0.495) + (Double3(1, 0, 1).normalized() * 0.1)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));

	DebugAssertMsg(B.CheckCollision(static_cast<const Collider3D&>(A), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(hitInfo.A == static_cast<const Collider3D*>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for A"));
	DebugAssertMsg(hitInfo.B == static_cast<const Collider3D*>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for B"));
	DebugAssertMsg(CloseEnough(hitInfo.Normal, Double3(-1, 0, -1).normalized()), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect Normal"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnA, Double3(0.495, 0, 0.495) + (Double3(1, 0, 1).normalized() * 0.1)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnB, Double3(0.5, 0, 0.5)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));
}

void UnitTestCylinder_Quad_Interpenetrating_NormalPositive_NegativeYEdge()
{
	QuadCollider3D A(Double3(0.5, 0.5, 0.5), Double3(-1, 0, -1), sqrt(2), 1);
	AxisAlignedCylinderCollider3D B(Double3(0.495, -0.49, 0.495), 0.1, 0.5);

	Collider3D::ColliderHit hitInfo(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);
	DebugAssertMsg(A.CheckCollision(static_cast<const Collider3D&>(B), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(hitInfo.A == static_cast<const Collider3D*>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for A"));
	DebugAssertMsg(hitInfo.B == static_cast<const Collider3D*>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for B"));
	DebugAssertMsg(CloseEnough(hitInfo.Normal, Double3(1, 0, 1).normalized()), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect Normal"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnA, Double3(0.5, 0, 0.5)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnB, Double3(0.495, 0, 0.495) + (Double3(1, 0, 1).normalized() * 0.1)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));

	DebugAssertMsg(B.CheckCollision(static_cast<const Collider3D&>(A), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(hitInfo.A == static_cast<const Collider3D*>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for A"));
	DebugAssertMsg(hitInfo.B == static_cast<const Collider3D*>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for B"));
	DebugAssertMsg(CloseEnough(hitInfo.Normal, Double3(-1, 0, -1).normalized()), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect Normal"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnA, Double3(0.495, 0, 0.495) + (Double3(1, 0, 1).normalized() * 0.1)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnB, Double3(0.5, 0, 0.5)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));
}

void UnitTestCylinder_Quad_NoCollision_NormalNegative_NegativeYEdge()
{
	QuadCollider3D A(Double3(0.5, 0.5, 0.5), Double3(-1, 0, -1), sqrt(2), 1);
	AxisAlignedCylinderCollider3D B(Double3(0.505, -0.51, 0.505), 0.1, 0.5);

	Collider3D::ColliderHit hitInfo(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);
	DebugAssertMsg(!A.CheckCollision(static_cast<const Collider3D&>(B), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(!B.CheckCollision(static_cast<const Collider3D&>(A), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
}

void UnitTestCylinder_Quad_Touching_NormalNegative_NegativeYEdge()
{
	QuadCollider3D A(Double3(0.5, 0.5, 0.5), Double3(-1, 0, -1), sqrt(2), 1);
	AxisAlignedCylinderCollider3D B(Double3(0.505, -0.5, 0.505), 0.1, 0.5);

	Collider3D::ColliderHit hitInfo(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);
	DebugAssertMsg(A.CheckCollision(static_cast<const Collider3D&>(B), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(hitInfo.A == static_cast<const Collider3D*>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for A"));
	DebugAssertMsg(hitInfo.B == static_cast<const Collider3D*>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for B"));
	DebugAssertMsg(CloseEnough(hitInfo.Normal, Double3(-1, 0, -1).normalized()), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect Normal"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnA, Double3(0.5, 0, 0.5)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnB, Double3(0.505, 0, 0.505) - (Double3(1, 0, 1).normalized() * 0.1)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));

	DebugAssertMsg(B.CheckCollision(static_cast<const Collider3D&>(A), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(hitInfo.A == static_cast<const Collider3D*>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for A"));
	DebugAssertMsg(hitInfo.B == static_cast<const Collider3D*>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for B"));
	DebugAssertMsg(CloseEnough(hitInfo.Normal, Double3(1, 0, 1).normalized()), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect Normal"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnA, Double3(0.505, 0, 0.505) - (Double3(1, 0, 1).normalized() * 0.1)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnB, Double3(0.5, 0, 0.5)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));
}

void UnitTestCylinder_Quad_Interpenetrating_NormalNegative_NegativeYEdge()
{
	QuadCollider3D A(Double3(0.5, 0.5, 0.5), Double3(-1, 0, -1), sqrt(2), 1);
	AxisAlignedCylinderCollider3D B(Double3(0.505, -0.49, 0.505), 0.1, 0.5);

	Collider3D::ColliderHit hitInfo(nullptr, nullptr, Double3::Zero, Double3::Zero, Double3::Zero);
	DebugAssertMsg(A.CheckCollision(static_cast<const Collider3D&>(B), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(hitInfo.A == static_cast<const Collider3D*>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for A"));
	DebugAssertMsg(hitInfo.B == static_cast<const Collider3D*>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for B"));
	DebugAssertMsg(CloseEnough(hitInfo.Normal, Double3(-1, 0, -1).normalized()), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect Normal"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnA, Double3(0.5, 0, 0.5)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnB, Double3(0.505,0 , 0.505) - (Double3(1, 0, 1).normalized() * 0.1)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));

	DebugAssertMsg(B.CheckCollision(static_cast<const Collider3D&>(A), hitInfo), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect return value"));
	DebugAssertMsg(hitInfo.A == static_cast<const Collider3D*>(&B), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for A"));
	DebugAssertMsg(hitInfo.B == static_cast<const Collider3D*>(&A), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for B"));
	DebugAssertMsg(CloseEnough(hitInfo.Normal, Double3(1, 0, 1).normalized()), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect Normal"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnA, Double3(0.505, 0, 0.505) - (Double3(1, 0, 1).normalized() * 0.1)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));
	DebugAssertMsg(CloseEnough(hitInfo.PointOfImpactOnB, Double3(0.5, 0, 0.5)), std::string("Failed ") + __FUNCTION__ + std::string(": Incorrect value for PointOfImpactOnA"));
}

#pragma endregion Unit Tests - Cylinder near Top/Bottom of Quad

#pragma region Unit Tests - Cylinder near Corners of Quad

#pragma endregion Unit Tests - Cylinder near Corners of Quad

#pragma endregion Unit Tests - Axis Aligned Cylinder -> Quad

void UnitTests_AABB_AABB()
{
	UnitTestAABB_AABB_NoCollision_PosX();
	UnitTestAABB_AABB_NoCollision_NegX();
	UnitTestAABB_AABB_NoCollision_PosY();
	UnitTestAABB_AABB_NoCollision_NegY();
	UnitTestAABB_AABB_NoCollision_PosZ();
	UnitTestAABB_AABB_NoCollision_NegZ();

	// @todo: Add edge cases
}

void UnitTests_Cylinder_Cylinder()
{
	// Check side collisions
	UnitTestCylinder_Cylinder_PosX_Interpenetrating();
	UnitTestCylinder_Cylinder_PosZ_Interpenetrating();
	UnitTestCylinder_Cylinder_NegX_Interpenetrating();
	UnitTestCylinder_Cylinder_NegZ_Interpenetrating();

	// Check top collisions along edge
	UnitTestCylinder_Cylinder_Top_PosX_Interpenetrating();
	UnitTestCylinder_Cylinder_Top_PosZ_Interpenetrating();
	UnitTestCylinder_Cylinder_Top_NegX_Interpenetrating();
	UnitTestCylinder_Cylinder_Top_NegZ_Interpenetrating();

	// Check bottom collisions along edge
	UnitTestCylinder_Cylinder_Bottom_PosX_Interpenetrating();
	UnitTestCylinder_Cylinder_Bottom_PosZ_Interpenetrating();
	UnitTestCylinder_Cylinder_Bottom_NegX_Interpenetrating();
	UnitTestCylinder_Cylinder_Bottom_NegZ_Interpenetrating();
}

void UnitTests_AABB_Cylinder()
{
	// Testing collisions along the Y axis
	UnitTestAABB_Cylinder_Above();
	UnitTestAABB_Cylinder_RestingOnTop();
	UnitTestAABB_Cylinder_InterpenetratingTop();
	UnitTestAABB_Cylinder_InterpenetratingTop_PosXPosZCorner();
	UnitTestAABB_Cylinder_InterpenetratingTop_PosZEdge_NearPosX();
	UnitTestAABB_Cylinder_InterpenetratingTop_PosZEdge();
	UnitTestAABB_Cylinder_InterpenetratingTop_PosZEdge_NearNegX();
	UnitTestAABB_Cylinder_InterpenetratingTop_NegXPosZCorner();
	UnitTestAABB_Cylinder_InterpenetratingTop_PosXEdge_NearPosZ();
	UnitTestAABB_Cylinder_InterpenetratingTop_PosXEdge();
	UnitTestAABB_Cylinder_InterpenetratingTop_PosXEdge_NearNegZ();
	UnitTestAABB_Cylinder_InterpenetratingTop_PosXNegZCorner();
	UnitTestAABB_Cylinder_InterpenetratingTop_NegZEdge_NearPosX();
	UnitTestAABB_Cylinder_InterpenetratingTop_NegZEdge();
	UnitTestAABB_Cylinder_InterpenetratingTop_NegZEdge_NearNegX();
	UnitTestAABB_Cylinder_InterpenetratingTop_NegXNegZCorner();
	UnitTestAABB_Cylinder_InterpenetratingTop_NegXEdge_NearPosZ();
	UnitTestAABB_Cylinder_InterpenetratingTop_NegXEdge();
	UnitTestAABB_Cylinder_InterpenetratingTop_NegXEdge_NearNegZ();
	UnitTestAABB_Cylinder_InterpenetratingBottom_PosXPosZCorner();
	UnitTestAABB_Cylinder_InterpenetratingBottom_PosZEdge_NearPosX();
	UnitTestAABB_Cylinder_InterpenetratingBottom_PosZEdge();
	UnitTestAABB_Cylinder_InterpenetratingBottom_PosZEdge_NearNegX();
	UnitTestAABB_Cylinder_InterpenetratingBottom_NegXPosZCorner();
	UnitTestAABB_Cylinder_InterpenetratingBottom_PosXEdge_NearPosZ();
	UnitTestAABB_Cylinder_InterpenetratingBottom_PosXEdge();
	UnitTestAABB_Cylinder_InterpenetratingBottom_PosXEdge_NearNegZ();
	UnitTestAABB_Cylinder_InterpenetratingBottom_PosXNegZCorner();
	UnitTestAABB_Cylinder_InterpenetratingBottom_NegZEdge_NearPosX();
	UnitTestAABB_Cylinder_InterpenetratingBottom_NegZEdge();
	UnitTestAABB_Cylinder_InterpenetratingBottom_NegZEdge_NearNegX();
	UnitTestAABB_Cylinder_InterpenetratingBottom_NegXNegZCorner();
	UnitTestAABB_Cylinder_InterpenetratingBottom_NegXEdge_NearPosZ();
	UnitTestAABB_Cylinder_InterpenetratingBottom_NegXEdge();
	UnitTestAABB_Cylinder_InterpenetratingBottom_NegXEdge_NearNegZ();
	UnitTestAABB_Cylinder_InterpenetratingBottom();
	UnitTestAABB_Cylinder_TouchingBottom();
	UnitTestAABB_Cylinder_Below();
	
	// Testing collisions along the X axis
	UnitTestAABB_Cylinder_PosX_Interpenetrating();
	UnitTestAABB_Cylinder_PosX_Touching();
	UnitTestAABB_Cylinder_PosX_NoCollision();
	UnitTestAABB_Cylinder_PosX_NearPosXPosZCorner_Interpenetrating();
	UnitTestAABB_Cylinder_PosX_NearPosXPosZCorner_Touching();
	UnitTestAABB_Cylinder_PosX_NearPosXPosZCorner_NoCollision();
	UnitTestAABB_Cylinder_PosX_NearPosXNegZCorner_Interpenetrating();
	UnitTestAABB_Cylinder_PosX_NearPosXNegZCorner_Touching();
	UnitTestAABB_Cylinder_PosX_NearPosXNegZCorner_NoCollision();
	UnitTestAABB_Cylinder_NegX_Interpenetrating();
	UnitTestAABB_Cylinder_NegX_Touching();
	UnitTestAABB_Cylinder_NegX_NoCollision();
	UnitTestAABB_Cylinder_NegX_NearNegXPosZCorner_Interpenetrating();
	UnitTestAABB_Cylinder_NegX_NearNegXPosZCorner_Touching();
	UnitTestAABB_Cylinder_NegX_NearNegXPosZCorner_NoCollision();
	UnitTestAABB_Cylinder_NegX_NearNegXNegZCorner_Interpenetrating();
	UnitTestAABB_Cylinder_NegX_NearNegXNegZCorner_Touching();
	UnitTestAABB_Cylinder_NegX_NearNegXNegZCorner_NoCollision();

	// Testing collisions along the Z axis
	UnitTestAABB_Cylinder_PosZ_Interpenetrating();
	UnitTestAABB_Cylinder_PosZ_Touching();
	UnitTestAABB_Cylinder_PosZ_NoCollision();
	UnitTestAABB_Cylinder_PosZ_NearPosXPosZCorner_Interpenetrating();
	UnitTestAABB_Cylinder_PosZ_NearPosXPosZCorner_Touching();
	UnitTestAABB_Cylinder_PosZ_NearPosXPosZCorner_NoCollision();
	UnitTestAABB_Cylinder_PosZ_NearNegXPosZCorner_Interpenetrating();
	UnitTestAABB_Cylinder_PosZ_NearNegXPosZCorner_Touching();
	UnitTestAABB_Cylinder_PosZ_NearNegXPosZCorner_NoCollision();
	UnitTestAABB_Cylinder_NegZ_Interpenetrating();
	UnitTestAABB_Cylinder_NegZ_Touching();
	UnitTestAABB_Cylinder_NegZ_NoCollision();
	UnitTestAABB_Cylinder_NegZ_NearPosXNegZCorner_Interpenetrating();
	UnitTestAABB_Cylinder_NegZ_NearPosXNegZCorner_Touching();
	UnitTestAABB_Cylinder_NegZ_NearPosXNegZCorner_NoCollision();
	UnitTestAABB_Cylinder_NegZ_NearNegXNegZCorner_Interpenetrating();
	UnitTestAABB_Cylinder_NegZ_NearNegXNegZCorner_Touching();
	UnitTestAABB_Cylinder_NegZ_NearNegXNegZCorner_NoCollision();

	// Test collisions at corners of box
	UnitTestAABB_Cylinder_PosXPosZCorner_Interpenetrating();
	UnitTestAABB_Cylinder_PosXPosZCorner_NoCollision();
	UnitTestAABB_Cylinder_PosXNegZCorner_Interpenetrating();
	UnitTestAABB_Cylinder_PosXNegZCorner_NoCollision();
	UnitTestAABB_Cylinder_NegXPosZCorner_Interpenetrating();
	UnitTestAABB_Cylinder_NegXPosZCorner_NoCollision();
	UnitTestAABB_Cylinder_NegXNegZCorner_Interpenetrating();
	UnitTestAABB_Cylinder_NegXNegZCorner_NoCollision();
}

void UnitTests_Cylinder_Quad()
{
	// Test collisions near center of quad
	UnitTestCylinder_Quad_NoCollision_NormalPositive();
	UnitTestCylinder_Quad_Touching_NormalPositive();
	UnitTestCylinder_Quad_Interpenetrating_NormalPositive();
	UnitTestCylinder_Quad_Interpenetrating_NormalNegative();
	UnitTestCylinder_Quad_Touching_NormalNegative();
	UnitTestCylinder_Quad_NoCollision_NormalNegative();

	// Test collisions near sides of quad
	UnitTestCylinder_Quad_NoCollision_NormalPositive_PositiveXEdge();
	UnitTestCylinder_Quad_Touching_NormalPositive_PositiveXEdge();
	UnitTestCylinder_Quad_Interpenetrating_NormalPositive_PositiveXEdge();
	UnitTestCylinder_Quad_NoCollision_NormalNegative_PositiveXEdge();
	UnitTestCylinder_Quad_Touching_NormalNegative_PositiveXEdge();
	UnitTestCylinder_Quad_Interpenetrating_NormalNegative_PositiveXEdge();

	UnitTestCylinder_Quad_NoCollision_NormalPositive_NegativeXEdge();
	UnitTestCylinder_Quad_Touching_NormalPositive_NegativeXEdge();
	UnitTestCylinder_Quad_Interpenetrating_NormalPositive_NegativeXEdge();
	UnitTestCylinder_Quad_NoCollision_NormalNegative_NegativeXEdge();
	UnitTestCylinder_Quad_Touching_NormalNegative_NegativeXEdge();
	UnitTestCylinder_Quad_Interpenetrating_NormalNegative_NegativeXEdge();

	// Test collisions near top/bottom of quad
	UnitTestCylinder_Quad_NoCollision_NormalPositive_PositiveYEdge();
	UnitTestCylinder_Quad_Touching_NormalPositive_PositiveYEdge();
	UnitTestCylinder_Quad_Interpenetrating_NormalPositive_PositiveYEdge();
	UnitTestCylinder_Quad_NoCollision_NormalNegative_PositiveYEdge();
	UnitTestCylinder_Quad_Touching_NormalNegative_PositiveYEdge();
	UnitTestCylinder_Quad_Interpenetrating_NormalNegative_PositiveYEdge();

	UnitTestCylinder_Quad_NoCollision_NormalPositive_NegativeYEdge();
	UnitTestCylinder_Quad_Touching_NormalPositive_NegativeYEdge();
	UnitTestCylinder_Quad_Interpenetrating_NormalPositive_NegativeYEdge();
	UnitTestCylinder_Quad_NoCollision_NormalNegative_NegativeYEdge();
	UnitTestCylinder_Quad_Touching_NormalNegative_NegativeYEdge();
	UnitTestCylinder_Quad_Interpenetrating_NormalNegative_NegativeYEdge();

	// TODO: Test collisions near corners of quad
}

void Collider3D::RunUnitTests()
{
#ifndef NDEBUG
	// AABB -> AABB Unit Tests
	UnitTests_AABB_AABB();

	// Cylinder -> Cylinder Unit Tests
	UnitTests_Cylinder_Cylinder();

	// AABB -> Cylinder Unit Tests
	UnitTests_AABB_Cylinder();

	// Cylinder -> Quad Unit Tests
	UnitTests_Cylinder_Quad();
#endif
}