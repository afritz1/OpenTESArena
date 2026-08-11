#pragma once

#include <vector>

#include "EntityInstance.h"
#include "../World/Chunk.h"

struct EntityChunk final : public Chunk
{
	// Weak references to entities physically present in this chunk, updated each frame immediately after physics update.
	// Intended for non-gameplay systems like visibility and rendering.
	std::vector<EntityInstanceID> entityIDs;

	void init(const ChunkInt2 &position, int height);
	void clear();
};
