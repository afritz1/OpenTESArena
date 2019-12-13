#ifndef PHYSICS_H
#define PHYSICS_H

#include "../Math/Vector2.h"
#include "../Math/Vector3.h"
#include "../World/VoxelData.h"
#include "../Entities/EntityManager.h"

// Static class for physics-related calculations like ray casting.

class VoxelGrid;

class Physics
{
public:
	// Intersection data for ray casts.
	struct Hit
	{
		enum class Type { Voxel, Entity };

		double t;
		Double3 point;
		Int3 voxel;
		VoxelData::Facing facing;
		Hit::Type type;
		union { uint16_t voxelID; int entityID; };
	};
private:
	Physics() = delete;
	~Physics() = delete;

	// Checks an initial voxel for ray hits and writes them into the output parameter.
	// Returns true if the ray hit something.
	static bool testInitialVoxelRay(const Double3 &rayStart, const Double3 &direction,
		const Int3 &voxel, VoxelData::Facing facing, const Double3 &nearPoint,
		const Double3 &farPoint, double ceilingHeigt, const VoxelGrid &voxelGrid,
		Physics::Hit &hit);

	// Checks a voxel for ray hits and writes them into the output parameter. Returns
	// true if the ray hit something.
	static bool testVoxelRay(const Double3 &rayStart, const Double3 &direction,
		const Int3 &voxel, VoxelData::Facing facing, const Double3 &nearPoint,
		const Double3 &farPoint, double ceilingHeight, const VoxelGrid &voxelGrid,
		Physics::Hit &hit);
public:
	// @todo: bit mask elements for each voxel data type.

	// Casts a ray through the world and writes any intersection data into the output
	// parameter. Returns true if the ray hit something.
	static bool rayCast(const Double3 &rayStart, const Double3 &direction, double ceilingHeight,
		const VoxelGrid &voxelGrid, const EntityManager& entityManager, Physics::Hit &hit);
	static bool rayCast(const Double3 &rayStart, const Double3 &direction,
		const VoxelGrid &voxelGrid, const EntityManager& entityManager, Physics::Hit &hit);
};

#endif
