#include "EntityChunk.h"

void EntityChunk::init(const ChunkInt2 &position, int height)
{
	Chunk::init(position, height);
}

void EntityChunk::clear()
{
	Chunk::clear();
	this->entityIDs.clear();
	this->addedEntityIDs.clear();
	this->removedEntityIDs.clear();
}
