#include "CollisionChunkManager.h"

void CollisionChunkManager::update(double dt, const BufferView<const ChunkInt2> &newChunkPositions,
	const BufferView<const ChunkInt2> &freedChunkPositions)
{
	for (int i = 0; i < freedChunkPositions.getCount(); i++)
	{
		const ChunkInt2 &chunkPos = freedChunkPositions.get(i);
		// @todo
		DebugNotImplemented();
	}

	for (int i = 0; i < newChunkPositions.getCount(); i++)
	{
		const ChunkInt2 &chunkPos = newChunkPositions.get(i);
		// @todo
		DebugNotImplemented();
	}
}
