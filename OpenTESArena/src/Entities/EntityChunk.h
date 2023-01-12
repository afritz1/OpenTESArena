#ifndef ENTITY_CHUNK_H
#define ENTITY_CHUNK_H

#include "../World/Chunk.h"

class EntityChunk final : public Chunk
{
public:
	// @todo: decide what should go in here.
	// - EntityChunk should not own the entity, only a reference of some kind (maybe a hash set of entity IDs for quick "is this entity touching this chunk?").

	// @todo: bounding box

	void init(const ChunkInt2 &position, int height);
	void clear();
};

#endif
