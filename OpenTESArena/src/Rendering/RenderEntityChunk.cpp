#include "RenderEntityChunk.h"

void RenderEntityChunk::init(const ChunkInt2 &position, int height)
{
	Chunk::init(position, height);
}

void RenderEntityChunk::clear()
{
	Chunk::clear();
	this->drawCalls.clear();
}
