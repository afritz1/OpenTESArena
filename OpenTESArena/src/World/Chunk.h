#ifndef CHUNK_H
#define CHUNK_H

#include "ChunkUtils.h"
#include "../Math/MathUtils.h"

// Base chunk for all other chunk types in the game world.
class Chunk
{
private:
	ChunkInt2 position;
	int height;
protected:	
	void init(const ChunkInt2 &position, int height);
	void clear();
public:
	static constexpr SNInt WIDTH = ChunkUtils::CHUNK_DIM;
	static constexpr WEInt DEPTH = WIDTH;
	static_assert(MathUtils::isPowerOf2(WIDTH));

	const ChunkInt2 &getPosition() const;
	int getHeight() const;
	bool isValidVoxel(SNInt x, int y, WEInt z) const;
};

#endif
