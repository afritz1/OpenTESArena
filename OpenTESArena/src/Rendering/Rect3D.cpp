#include <algorithm>
#include <cassert>

#include "Rect3D.h"

Rect3D::Rect3D(const Float3 &p1, const Float3 &p2, const Float3 &p3)
	: p1(p1), p2(p2), p3(p3) { }

Rect3D::~Rect3D()
{

}

Rect3D Rect3D::fromFrame(const Float3 &point, const Float3 &right,
	const Float3 &up, float width, float height)
{
	assert(right.isNormalized());
	assert(up.isNormalized());

	// Right and up diff vectors that determine how big the rectangle is.
	const Float3 dR = right * (width * 0.5f);
	const Float3 dU = up * height;

	const Float3 p1 = point + dR + dU;
	const Float3 p2 = point + dR;
	const Float3 p3 = point - dR;

	return Rect3D(p1, p2, p3);
}

const Float3 &Rect3D::getP1() const
{
	return this->p1;
}

const Float3 &Rect3D::getP2() const
{
	return this->p2;
}

const Float3 &Rect3D::getP3() const
{
	return this->p3;
}

Float3 Rect3D::getP4() const
{
	return this->p1 + (this->p3 - this->p2);
}

Float3 Rect3D::getNormal() const
{
	const Float3 p1p2 = this->p2 - this->p1;
	const Float3 p1p3 = this->p3 - this->p1;
	return p1p2.cross(p1p3).normalized();
}

std::pair<Float3, Float3> Rect3D::getAABB() const
{
	const Float3 p4 = this->getP4();

	const float minX = std::min(this->p1.x, std::min(this->p2.x,
		std::min(this->p3.x, p4.x)));
	const float minY = std::min(this->p1.y, std::min(this->p2.y,
		std::min(this->p3.y, p4.y)));
	const float minZ = std::min(this->p1.z, std::min(this->p2.z,
		std::min(this->p3.z, p4.z)));

	const float maxX = std::max(this->p1.x, std::max(this->p2.x,
		std::max(this->p3.x, p4.x)));
	const float maxY = std::max(this->p1.y, std::max(this->p2.y,
		std::max(this->p3.y, p4.y)));
	const float maxZ = std::max(this->p1.z, std::max(this->p2.z,
		std::max(this->p3.z, p4.z)));

	return std::make_pair(
		Float3(minX, minY, minZ),
		Float3(maxX, maxY, maxZ));
}

std::vector<Int3> Rect3D::getTouchedVoxels(int worldWidth, int worldHeight, 
	int worldDepth) const
{
	// Create an axis-aligned bounding box for the rectangle.
	const std::pair<Float3, Float3> aabb = this->getAABB();
	const Float3 &boxMin = aabb.first;
	const Float3 &boxMax = aabb.second;

	// Lambda to convert a 3D point to a voxel coordinate clamped within 
	// world bounds.
	auto getClampedCoordinate = [worldWidth, worldHeight, worldDepth](const Float3 &point)
	{
		const int pX = static_cast<int>(point.x);
		const int pY = static_cast<int>(point.y);
		const int pZ = static_cast<int>(point.z);

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
	for (int k = voxelMin.z; k <= voxelMax.z; ++k)
	{
		for (int j = voxelMin.y; j <= voxelMax.y; ++j)
		{
			for (int i = voxelMin.x; i <= voxelMax.x; ++i)
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
	std::pair<Float3, Float3> aabb = this->getAABB();
	const Float3 &boxMin = aabb.first;
	const Float3 &boxMax = aabb.second;

	// Lambda to convert a 3D point to a voxel coordinate.
	auto getVoxelCoordinate = [](const Float3 &point)
	{
		return Int3(
			static_cast<int>(point.x), 
			static_cast<int>(point.y),
			static_cast<int>(point.z));
	};

	// Voxel coordinates for the nearest and farthest corners from the origin.
	const Int3 voxelMin = getVoxelCoordinate(boxMin);
	const Int3 voxelMax = getVoxelCoordinate(boxMax);

	// Insert a 3D coordinate for every voxel the bounding box touches.
	std::vector<Int3> coordinates;
	for (int k = voxelMin.z; k <= voxelMax.z; ++k)
	{
		for (int j = voxelMin.y; j <= voxelMax.y; ++j)
		{
			for (int i = voxelMin.x; i <= voxelMax.x; ++i)
			{
				coordinates.push_back(Int3(i, j, k));
			}
		}
	}

	return coordinates;
}
