#ifndef RAY_CAST_TYPES_H
#define RAY_CAST_TYPES_H

#include <limits>

#include "../Entities/EntityInstance.h"
#include "../Math/Vector3.h"
#include "../Voxels/VoxelUtils.h"

enum class RayCastHitType
{
	Voxel,
	Entity
};

struct RayCastVoxelHit
{
	VoxelInt3 voxel;
	VoxelFacing3D facing;

	RayCastVoxelHit();
};

struct RayCastEntityHit
{
	EntityInstanceID id;

	RayCastEntityHit();
};

// Intersection data for ray casts.
struct RayCastHit
{
	static constexpr double NO_HIT_DISTANCE = std::numeric_limits<double>::infinity();

	double t; // Distance from ray start.
	CoordDouble3 coord; // Hit point in the scene.
	
	RayCastHitType type;
	RayCastVoxelHit voxelHit;
	RayCastEntityHit entityHit;

	RayCastHit();

	void initVoxel(double t, const CoordDouble3 &coord, const VoxelInt3 &voxel, VoxelFacing3D facing);
	void initEntity(double t, const CoordDouble3 &coord, EntityInstanceID id);
};

#endif
