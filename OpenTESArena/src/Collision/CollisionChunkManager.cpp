#include "CollisionChunkManager.h"
#include "../Voxels/VoxelChunkManager.h"

void CollisionChunkManager::populateChunk(int index, const ChunkInt2 &chunkPos,
	const VoxelChunk &voxelChunk)
{
	const int chunkHeight = voxelChunk.getHeight();
	CollisionChunk &collisionChunk = this->getChunkAtIndex(index);
	collisionChunk.init(chunkPos, chunkHeight);

	std::unordered_map<VoxelChunk::VoxelMeshDefID, CollisionChunk::CollisionMeshDefID> meshMappings;

	for (WEInt z = 0; z < Chunk::DEPTH; z++)
	{
		for (int y = 0; y < chunkHeight; y++)
		{
			for (SNInt x = 0; x < Chunk::WIDTH; x++)
			{
				// Colliders are dependent on the voxel mesh definition.
				const VoxelChunk::VoxelMeshDefID voxelMeshDefID = voxelChunk.getMeshDefID(x, y, z);
				const auto iter = meshMappings.find(voxelMeshDefID);

				CollisionChunk::CollisionMeshDefID collisionMeshDefID = -1;
				if (iter != meshMappings.end())
				{
					collisionMeshDefID = iter->second;
				}
				else
				{
					const VoxelMeshDefinition &voxelMeshDef = voxelChunk.getMeshDef(voxelMeshDefID);
					const BufferView<const double> verticesView(voxelMeshDef.collisionVertices.data(), static_cast<int>(voxelMeshDef.collisionVertices.size()));
					const BufferView<const double> normalsView(voxelMeshDef.collisionNormals.data(), static_cast<int>(voxelMeshDef.collisionNormals.size()));
					const BufferView<const int> indicesView(voxelMeshDef.collisionIndices.data(), static_cast<int>(voxelMeshDef.collisionIndices.size()));

					CollisionMeshDefinition collisionMeshDef;
					collisionMeshDef.init(verticesView, normalsView, indicesView);
					collisionMeshDefID = collisionChunk.addCollisionMeshDef(std::move(collisionMeshDef));
					meshMappings.emplace(voxelMeshDefID, collisionMeshDefID);
				}

				collisionChunk.setCollisionMeshDefID(x, y, z, collisionMeshDefID);
				collisionChunk.setColliderEnabled(x, y, z, true);
			}
		}
	}
}

void CollisionChunkManager::update(double dt, const BufferView<const ChunkInt2> &newChunkPositions,
	const BufferView<const ChunkInt2> &freedChunkPositions, const VoxelChunkManager &voxelChunkManager)
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

	this->chunkPool.clear();
}
