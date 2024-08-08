#ifndef PHYSICS_H
#define PHYSICS_H

#include "../Entities/EntityInstance.h"
#include "../Math/Vector2.h"
#include "../Math/Vector3.h"
#include "../Voxels/VoxelUtils.h"

class CollisionChunkManager;
class EntityChunkManager;
class VoxelChunkManager;

struct RayCastHit;

namespace Physics
{
	// @todo: bit mask elements for each voxel type.

	// Casts a ray through the world and writes any intersection data into the output parameter. Returns true
	// if the ray hit something.
	bool rayCast(const CoordDouble3 &rayStart, const VoxelDouble3 &rayDirection, double ceilingScale,
		const VoxelDouble3 &cameraForward, bool includeEntities, const VoxelChunkManager &voxelChunkManager,
		const EntityChunkManager &entityChunkManager, const CollisionChunkManager &collisionChunkManager,
		const EntityDefinitionLibrary &entityDefLibrary, RayCastHit &hit);
	bool rayCast(const CoordDouble3 &rayStart, const VoxelDouble3 &rayDirection, const VoxelDouble3 &cameraForward,
		bool includeEntities, const VoxelChunkManager &voxelChunkManager, const EntityChunkManager &entityChunkManager,
		const CollisionChunkManager &collisionChunkManager, const EntityDefinitionLibrary &entityDefLibrary,
		RayCastHit &hit);
};

#endif
