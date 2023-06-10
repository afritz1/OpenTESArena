#ifndef CHUNK_MANAGER_H
#define CHUNK_MANAGER_H

#include <optional>
#include <vector>

#include "Coord.h"

#include "components/utilities/BufferView.h"

class ChunkManager
{
private:
	std::vector<ChunkInt2> activeChunkPositions; // Active this frame.
	std::vector<ChunkInt2> newChunkPositions; // Spawned this frame (a subset of the active ones).
	std::vector<ChunkInt2> freedChunkPositions; // Freed this frame (no longer in the active ones).
	int centerChunkPosIndex; // Current center of the world.
public:
	ChunkManager();

	BufferView<const ChunkInt2> getActiveChunkPositions() const;
	BufferView<const ChunkInt2> getNewChunkPositions() const;
	BufferView<const ChunkInt2> getFreedChunkPositions() const;
	int getCenterChunkIndex() const;
	std::optional<int> tryGetChunkIndex(const ChunkInt2 &position) const;

	void update(const ChunkInt2 &centerChunkPos, int chunkDistance);
	void cleanUp();
	void clear();
};

#endif
