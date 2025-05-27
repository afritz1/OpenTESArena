#ifndef COLLISION_CHUNK_MANAGER_H
#define COLLISION_CHUNK_MANAGER_H

#include "Jolt/Jolt.h"
#include "Jolt/Physics/PhysicsSystem.h"

#include "CollisionChunk.h"
#include "../World/Coord.h"
#include "../World/SpecializedChunkManager.h"

class VoxelChunk;
class VoxelChunkManager;

// Handles the lifetimes of collision chunks.
class CollisionChunkManager final : public SpecializedChunkManager<CollisionChunk>
{
private:
	void populateChunkShapeDefs(CollisionChunk &collisionChunk, const VoxelChunk &voxelChunk);
	void populateChunkEnabledColliders(CollisionChunk &collisionChunk, const VoxelChunk &voxelChunk);
	void populateChunk(int index, double ceilingScale, const ChunkInt2 &chunkPos, const VoxelChunk &voxelChunk, JPH::PhysicsSystem &physicsSystem);
	void updateDirtyVoxels(const ChunkInt2 &chunkPos, double ceilingScale, const VoxelChunk &voxelChunk, JPH::PhysicsSystem &physicsSystem);
public:
	void update(double dt, BufferView<const ChunkInt2> activeChunkPositions, BufferView<const ChunkInt2> newChunkPositions,
		BufferView<const ChunkInt2> freedChunkPositions, double ceilingScale, const VoxelChunkManager &voxelChunkManager,
		JPH::PhysicsSystem &physicsSystem);

	void clear(JPH::PhysicsSystem &physicsSystem);
};

#endif
