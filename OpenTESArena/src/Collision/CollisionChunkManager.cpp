#include "CollisionChunkManager.h"
#include "../Voxels/VoxelChunkManager.h"

void CollisionChunkManager::populateChunk(int index, const ChunkInt2 &chunkPos, const VoxelChunk &voxelChunk)
{
	const int chunkHeight = voxelChunk.getHeight();
	CollisionChunk &collisionChunk = this->getChunkAtIndex(index);
	collisionChunk.init(chunkPos, chunkHeight);

	for (WEInt z = 0; z < Chunk::DEPTH; z++)
	{
		for (int y = 0; y < chunkHeight; y++)
		{
			for (SNInt x = 0; x < Chunk::WIDTH; x++)
			{
				// Colliders are dependent on the voxel mesh definition.
				const VoxelChunk::VoxelMeshDefID voxelMeshDefID = voxelChunk.getMeshDefID(x, y, z);
				const CollisionChunk::CollisionMeshDefID collisionMeshDefID = collisionChunk.getOrAddMeshDefIdMapping(voxelChunk, voxelMeshDefID);
				collisionChunk.meshDefIDs.set(x, y, z, collisionMeshDefID);
				collisionChunk.enabledColliders.set(x, y, z, true);
			}
		}
	}
}

void CollisionChunkManager::updateDirtyVoxels(const ChunkInt2 &chunkPos, const VoxelChunk &voxelChunk)
{
	CollisionChunk &collisionChunk = this->getChunkAtPosition(chunkPos);

	const int dirtyMeshDefPosCount = voxelChunk.getDirtyMeshDefPositionCount();
	for (int i = 0; i < dirtyMeshDefPosCount; i++)
	{
		const VoxelInt3 &voxelPos = voxelChunk.getDirtyMeshDefPosition(i);
		const VoxelChunk::VoxelMeshDefID voxelMeshDefID = voxelChunk.getMeshDefID(voxelPos.x, voxelPos.y, voxelPos.z);
		const CollisionChunk::CollisionMeshDefID collisionMeshDefID = collisionChunk.getOrAddMeshDefIdMapping(voxelChunk, voxelMeshDefID);
		collisionChunk.meshDefIDs.set(voxelPos.x, voxelPos.y, voxelPos.z, collisionMeshDefID);
		collisionChunk.enabledColliders.set(voxelPos.x, voxelPos.y, voxelPos.z, true);
	}

	const int dirtyDoorAnimInstPosCount = voxelChunk.getDirtyDoorAnimInstPositionCount();
	for (int i = 0; i < dirtyDoorAnimInstPosCount; i++)
	{
		const VoxelInt3 &voxelPos = voxelChunk.getDirtyDoorAnimInstPosition(i);

		// The door anim inst might've been destroyed because it finished closing.
		int doorAnimInstIndex;
		const bool shouldEnableDoorCollider = !voxelChunk.tryGetDoorAnimInstIndex(voxelPos.x, voxelPos.y, voxelPos.z, &doorAnimInstIndex);
		collisionChunk.enabledColliders.set(voxelPos.x, voxelPos.y, voxelPos.z, shouldEnableDoorCollider);
	}
}

void CollisionChunkManager::update(double dt, const BufferView<const ChunkInt2> &activeChunkPositions,
	const BufferView<const ChunkInt2> &newChunkPositions, const BufferView<const ChunkInt2> &freedChunkPositions,
	const VoxelChunkManager &voxelChunkManager)
{
	for (int i = 0; i < freedChunkPositions.getCount(); i++)
	{
		const ChunkInt2 &chunkPos = freedChunkPositions.get(i);
		const int chunkIndex = this->getChunkIndex(chunkPos);
		this->recycleChunk(chunkIndex);
	}

	for (int i = 0; i < newChunkPositions.getCount(); i++)
	{
		const ChunkInt2 &chunkPos = newChunkPositions.get(i);
		const int spawnIndex = this->spawnChunk();
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);
		this->populateChunk(spawnIndex, chunkPos, voxelChunk);
	}

	// Update dirty voxels.
	for (int i = 0; i < activeChunkPositions.getCount(); i++)
	{
		const ChunkInt2 &chunkPos = activeChunkPositions.get(i);
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);
		this->updateDirtyVoxels(chunkPos, voxelChunk);
	}

	this->chunkPool.clear();
}
