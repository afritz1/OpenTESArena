#ifndef ENTITY_CHUNK_H
#define ENTITY_CHUNK_H

#include <vector>

#include "EntityInstance.h"
#include "../World/Chunk.h"

struct EntityChunk final : public Chunk
{
	// Entities physically present in this chunk, based on their center position. Owned by EntityChunkManager.
	std::vector<EntityInstanceID> entityIDs;

	void init(const ChunkInt2 &position, int height);
	void clear();
};

#endif
