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
	CoordInt3 voxelCoord;
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
	WorldDouble3 worldPoint; // Hit point in the scene. Don't use this to determine the hit voxel since the collision mesh might z-fight with the boundaries of its voxel.
	
	RayCastHitType type;
	RayCastVoxelHit voxelHit;
	RayCastEntityHit entityHit;

	RayCastHit();

	void initVoxel(double t, const WorldDouble3 &worldPoint, const CoordInt3 &voxelCoord, VoxelFacing3D facing);
	void initEntity(double t, const WorldDouble3 &worldPoint, EntityInstanceID id);
};

#endif
