#ifndef RENDER_ENTITY_CHUNK_H
#define RENDER_ENTITY_CHUNK_H

#include <vector>

#include "RenderDrawCall.h"
#include "../World/Chunk.h"

class RenderEntityChunk final : public Chunk
{
public:
	std::vector<RenderDrawCall> drawCalls;

	void init(const ChunkInt2 &position, int height);
	void clear();
};

#endif
