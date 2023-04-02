#ifndef ENTITY_CHUNK_H
#define ENTITY_CHUNK_H

#include <vector>

#include "EntityInstance.h"
#include "../World/Chunk.h"

class EntityChunk final : public Chunk
{
public:
	// Entities physically present in this chunk, based on their center position. Owned by EntityChunkManager.
	std::vector<EntityInstanceID> entityIDs;

	// Entities added to/removed from this chunk this frame, a subset of this chunk's total IDs. Cleared at end-of-frame.
	std::vector<EntityInstanceID> addedEntityIDs;
	std::vector<EntityInstanceID> removedEntityIDs;

	// @todo: it's important for this to store references to entities so that when this chunk is freed, all those entities can
	// be iterated for removal in EntityChunkManager.

	// @todo: bounding box (CoordDouble3 x 2) based on view-independent bounding box sum of all entities present,
	// recalculated every frame.

	void init(const ChunkInt2 &position, int height);
	void clear();
};

#endif
