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

	for (const VoxelInt3 &voxelPos : voxelChunk.getDirtyMeshDefPositions())
	{
		const VoxelChunk::VoxelMeshDefID voxelMeshDefID = voxelChunk.getMeshDefID(voxelPos.x, voxelPos.y, voxelPos.z);
		const CollisionChunk::CollisionMeshDefID collisionMeshDefID = collisionChunk.getOrAddMeshDefIdMapping(voxelChunk, voxelMeshDefID);
		collisionChunk.meshDefIDs.set(voxelPos.x, voxelPos.y, voxelPos.z, collisionMeshDefID);
		collisionChunk.enabledColliders.set(voxelPos.x, voxelPos.y, voxelPos.z, true);
	}

	for (const VoxelInt3 &voxelPos : voxelChunk.getDirtyDoorAnimInstPositions())
	{
		int doorAnimInstIndex;
		const bool success = voxelChunk.tryGetDoorAnimInstIndex(voxelPos.x, voxelPos.y, voxelPos.z, &doorAnimInstIndex);
		DebugAssertMsg(success, "Expected door anim inst to be available for (" + voxelPos.toString() + ").");
		
		const BufferView<const VoxelDoorAnimationInstance> doorAnimInsts = voxelChunk.getDoorAnimInsts();
		const VoxelDoorAnimationInstance &doorAnimInst = doorAnimInsts[doorAnimInstIndex];
		const bool shouldEnableDoorCollider = doorAnimInst.stateType == VoxelDoorAnimationInstance::StateType::Closed;
		collisionChunk.enabledColliders.set(voxelPos.x, voxelPos.y, voxelPos.z, shouldEnableDoorCollider);
	}
}

void CollisionChunkManager::update(double dt, BufferView<const ChunkInt2> activeChunkPositions,
	BufferView<const ChunkInt2> newChunkPositions, BufferView<const ChunkInt2> freedChunkPositions,
	const VoxelChunkManager &voxelChunkManager)
{
	for (const ChunkInt2 &chunkPos : freedChunkPositions)
	{
		const int chunkIndex = this->getChunkIndex(chunkPos);
		this->recycleChunk(chunkIndex);
	}

	for (const ChunkInt2 &chunkPos : newChunkPositions)
	{
		const int spawnIndex = this->spawnChunk();
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);
		this->populateChunk(spawnIndex, chunkPos, voxelChunk);
	}

	// Update dirty voxels.
	for (const ChunkInt2 &chunkPos : activeChunkPositions)
	{
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);
		this->updateDirtyVoxels(chunkPos, voxelChunk);
	}

	this->chunkPool.clear();
}
