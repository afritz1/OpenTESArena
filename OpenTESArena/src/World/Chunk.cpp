#include "Chunk.h"

void Chunk::init(const ChunkInt2 &position, int height)
{
	this->position = position;
	this->height = height;
}

void Chunk::clear()
{
	this->position = ChunkInt2();
	this->height = 0;
}

bool Chunk::isValidVoxel(SNInt x, int y, WEInt z) const
{
	return (x >= 0) && (x < Chunk::WIDTH) && (y >= 0) && (y < this->height) && (z >= 0) && (z < Chunk::DEPTH);
}
