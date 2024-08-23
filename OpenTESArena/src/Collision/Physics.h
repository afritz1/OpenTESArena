#ifndef PHYSICS_H
#define PHYSICS_H

#include "Jolt/Jolt.h"
#include "Jolt/Physics/Body/BodyID.h"

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
	// Jolt init values.
	constexpr int TempAllocatorByteCount = 20 * 1024 * 1024; // 20MB
	constexpr int ThreadCount = 1;
	constexpr int MaxBodies = 250000;
	constexpr int BodyMutexCount = 0; // Use default settings.
	constexpr int MaxBodyPairs = 65536;
	constexpr int MaxContactConstraints = 16384;
	constexpr double DeltaTime = 1.0 / 60.0; // Determines # of steps to run inside PhysicsSystem::update().

	const JPH::BodyID INVALID_BODY_ID;

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
