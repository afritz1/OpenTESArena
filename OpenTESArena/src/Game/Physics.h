#ifndef PHYSICS_H
#define PHYSICS_H

#include "../Math/Vector3.h"

// Static class for physics-related calculations like ray casting.

class VoxelGrid;

class Physics
{
public:
	// Intersection data for ray casts.
	struct Hit
	{
		double t;
		Double3 point, normal;
		Int3 voxel;
		// To do: type of intersection (voxel/entity). Probably with a union?
	};
private:
	Physics() = delete;
	~Physics() = delete;
public:
	// Casts a ray through the world and writes any intersection data into the output
	// parameter. Returns true if the ray hit something.
	bool rayCast(const Double3 &point, const Double3 &direction, double ceilingHeight,
		const VoxelGrid &voxelGrid, Physics::Hit &hit);
	bool rayCast(const Double3 &point, const Double3 &direction,
		const VoxelGrid &voxelGrid, Physics::Hit &hit);
};

#endif
