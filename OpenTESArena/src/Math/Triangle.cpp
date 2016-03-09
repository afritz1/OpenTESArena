#include <cassert>

#include "Triangle.h"

Triangle::Triangle(const Vector3d &p1, const Vector3d &p2, const Vector3d &p3,
	const Vector2d &uv1, const Vector2d &uv2, const Vector2d &uv3)
{
	this->p1 = p1;
	this->p2 = p2;
	this->p3 = p3;
	this->uv1 = uv1;
	this->uv2 = uv2;
	this->uv3 = uv3;
}

Triangle::~Triangle()
{

}

const Vector3d &Triangle::getP1() const
{
	return this->p1;
}

const Vector3d &Triangle::getP2() const
{
	return this->p2;
}

const Vector3d &Triangle::getP3() const
{
	return this->p3;
}

const Vector2d &Triangle::getUV1() const
{
	return this->uv1;
}

const Vector2d &Triangle::getUV2() const
{
	return this->uv2;
}

const Vector2d &Triangle::getUV3() const
{
	return this->uv3;
}

Vector3d Triangle::getNormal() const
{
	auto edge1 = this->p2 - this->p1;
	auto edge2 = this->p3 - this->p1;
	return edge1.cross(edge2).normalized();
}

Vector3d Triangle::getTangent() const
{
	// Not so simple. Needs texture coordinates to work for any orientation (like
	// with cube faces).

	// From:
	// http://gamedev.stackexchange.com/questions/68612/how-to-compute-tangent-and-bitangent-vectors
	// Also look at http://stackoverflow.com/questions/5255806/how-to-calculate-tangent-and-binormal.

	auto x1 = this->p2.getX() - this->p1.getX();
	auto x2 = this->p3.getX() - this->p1.getX();
	auto y1 = this->p2.getY() - this->p1.getY();
	auto y2 = this->p3.getY() - this->p1.getY();
	auto z1 = this->p2.getZ() - this->p1.getZ();
	auto z2 = this->p3.getZ() - this->p1.getZ();

	auto s1 = this->uv2.getX() - this->uv1.getX();
	auto s2 = this->uv3.getX() - this->uv1.getX();
	auto t1 = this->uv2.getY() - this->uv1.getY();
	auto t2 = this->uv3.getY() - this->uv1.getY();

	auto r = 1.0 / ((s1 * t2) - (s2 * t1));

	auto sDirection = Vector3d(
		((t2 * x1) - (t1 * x2)) * r,
		((t2 * y1) - (t1 * y2)) * r,
		((t2 * z1) - (t1 * z2)) * r);

	// Might not be correct. No Gram-Schmidt yet. Verify correctness in practice!
	return sDirection.normalized();
}

Vector3d Triangle::getBitangent() const
{
	// From:
	// http://gamedev.stackexchange.com/questions/68612/how-to-compute-tangent-and-bitangent-vectors
	// Also look at http://stackoverflow.com/questions/5255806/how-to-calculate-tangent-and-binormal.

	// I don't think doing normal.cross(tangent) is enough here, because it doesn't
	// account for texture coordinates, does it?

	auto x1 = this->p2.getX() - this->p1.getX();
	auto x2 = this->p3.getX() - this->p1.getX();
	auto y1 = this->p2.getY() - this->p1.getY();
	auto y2 = this->p3.getY() - this->p1.getY();
	auto z1 = this->p2.getZ() - this->p1.getZ();
	auto z2 = this->p3.getZ() - this->p1.getZ();

	auto s1 = this->uv2.getX() - this->uv1.getX();
	auto s2 = this->uv3.getX() - this->uv1.getX();
	auto t1 = this->uv2.getY() - this->uv1.getY();
	auto t2 = this->uv3.getY() - this->uv1.getY();

	auto r = 1.0 / ((s1 * t2) - (s2 * t1));

	auto tDirection = Vector3d(
		((s1 * x2) - (s2 * x1)) * r,
		((s1 * y2) - (s2 * y1)) * r,
		((s1 * z2) - (s2 * z1)) * r);

	// Might not be correct. No Gram-Schmidt yet. Verify correctness in practice!
	return tDirection.normalized();
}
