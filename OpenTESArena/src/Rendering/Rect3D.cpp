#include <algorithm>
#include <cassert>

#include "Rect3D.h"

#include "../Math/Int3.h"

Rect3D::Rect3D(const Float3f &p1, const Float3f &p2, const Float3f &p3)
	: p1(p1), p2(p2), p3(p3) { }

Rect3D::~Rect3D()
{

}

Rect3D Rect3D::fromFrame(const Float3f &point, const Float3f &right,
	const Float3f &up, float width, float height)
{
	assert(right.isNormalized());
	assert(up.isNormalized());

	// Right and up diff vectors that determine how big the rectangle is.
	const Float3f dR = right * (width * 0.5f);
	const Float3f dU = up * height;

	const Float3f p1 = point + dR + dU;
	const Float3f p2 = point + dR;
	const Float3f p3 = point - dR;

	return Rect3D(p1, p2, p3);
}

const Float3f &Rect3D::getP1() const
{
	return this->p1;
}

const Float3f &Rect3D::getP2() const
{
	return this->p2;
}

const Float3f &Rect3D::getP3() const
{
	return this->p3;
}

Float3f Rect3D::getP4() const
{
	return this->p1 + (this->p3 - this->p2);
}

Float3f Rect3D::getNormal() const
{
	const Float3f p1p2 = this->p2 - this->p1;
	const Float3f p1p3 = this->p3 - this->p1;
	return p1p2.cross(p1p3).normalized();
}

std::pair<Float3f, Float3f> Rect3D::getAABB() const
{
	const Float3f p4 = this->getP4();

	const float minX = std::min(this->p1.getX(), std::min(this->p2.getX(),
		std::min(this->p3.getX(), p4.getX())));
	const float minY = std::min(this->p1.getY(), std::min(this->p2.getY(),
		std::min(this->p3.getY(), p4.getY())));
	const float minZ = std::min(this->p1.getZ(), std::min(this->p2.getZ(),
		std::min(this->p3.getZ(), p4.getZ())));

	const float maxX = std::max(this->p1.getX(), std::max(this->p2.getX(),
		std::max(this->p3.getX(), p4.getX())));
	const float maxY = std::max(this->p1.getY(), std::max(this->p2.getY(),
		std::max(this->p3.getY(), p4.getY())));
	const float maxZ = std::max(this->p1.getZ(), std::max(this->p2.getZ(),
		std::max(this->p3.getZ(), p4.getZ())));

	return std::make_pair(
		Float3f(minX, minY, minZ),
		Float3f(maxX, maxY, maxZ));
}

std::vector<Int3> Rect3D::getTouchedVoxels(int worldWidth, int worldHeight, 
	int worldDepth) const
{
	// Create an axis-aligned bounding box for the rectangle.
	const std::pair<Float3f, Float3f> aabb = this->getAABB();
	const Float3f &boxMin = aabb.first;
	const Float3f &boxMax = aabb.second;

	// Lambda to convert a 3D point to a voxel coordinate clamped within 
	// world bounds.
	auto getClampedCoordinate = [worldWidth, worldHeight, worldDepth](const Float3f &point)
	{
		const int pX = static_cast<int>(point.getX());
		const int pY = static_cast<int>(point.getY());
		const int pZ = static_cast<int>(point.getZ());

		return Int3(
			std::max(0, std::min(worldWidth - 1, pX)),
			std::max(0, std::min(worldHeight - 1, pY)),
			std::max(0, std::min(worldDepth - 1, pZ)));
	};

	// Voxel coordinates for the nearest and farthest corners from the origin.
	const Int3 voxelMin = getClampedCoordinate(boxMin);
	const Int3 voxelMax = getClampedCoordinate(boxMax);

	// Insert a 3D coordinate for every voxel the bounding box touches.
	std::vector<Int3> coordinates;
	for (int k = voxelMin.getZ(); k <= voxelMax.getZ(); ++k)
	{
		for (int j = voxelMin.getY(); j <= voxelMax.getY(); ++j)
		{
			for (int i = voxelMin.getX(); i <= voxelMax.getX(); ++i)
			{
				coordinates.push_back(Int3(i, j, k));
			}
		}
	}

	return coordinates;

	// This "bounding box" method sometimes gives false positives (resulting in 
	// wasted time checking an unrelated voxel) because when a sprite covers three 
	// voxels in an L, the bounding box will incorrectly cover a fourth voxel even 
	// though the sprite itself isn't touching it. It won't result in incorrect
	// behavior though; just less than optimal rectangle bounds.

	// For the more accurate method, this algorithm can assume that the sprite's normal
	// is always perpendicular to the global up, therefore allowing its geometry to be 
	// treated like a 2D line in the XZ plane. Maybe do either ray casting or Bresenham's 
	// from the top-down view using p2 to p3 on the rectangle, and copy the resulting
	// coordinates for each level of Y from the bottom up?
}

std::vector<Int3> Rect3D::getTouchedVoxels() const
{
	// Create an axis-aligned bounding box for the rectangle.
	std::pair<Float3f, Float3f> aabb = this->getAABB();
	const Float3f &boxMin = aabb.first;
	const Float3f &boxMax = aabb.second;

	// Lambda to convert a 3D point to a voxel coordinate.
	auto getVoxelCoordinate = [](const Float3f &point)
	{
		return Int3(
			static_cast<int>(point.getX()), 
			static_cast<int>(point.getY()),
			static_cast<int>(point.getZ()));
	};

	// Voxel coordinates for the nearest and farthest corners from the origin.
	const Int3 voxelMin = getVoxelCoordinate(boxMin);
	const Int3 voxelMax = getVoxelCoordinate(boxMax);

	// Insert a 3D coordinate for every voxel the bounding box touches.
	std::vector<Int3> coordinates;
	for (int k = voxelMin.getZ(); k <= voxelMax.getZ(); ++k)
	{
		for (int j = voxelMin.getY(); j <= voxelMax.getY(); ++j)
		{
			for (int i = voxelMin.getX(); i <= voxelMax.getX(); ++i)
			{
				coordinates.push_back(Int3(i, j, k));
			}
		}
	}

	return coordinates;
}
