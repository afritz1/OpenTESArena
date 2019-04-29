#include <algorithm>

#include "ChunkSet.h"

ChunkSet::ChunkSet(bool exterior, bool wrap)
{
	this->exterior = exterior;
	this->wrap = wrap;
}

auto ChunkSet::getIter(int x, int y)
{
	return std::find_if(this->chunks.begin(), this->chunks.end(),
		[x, y](const std::unique_ptr<Chunk> &chunk)
	{
		return (chunk->getX() == x) && (chunk->getY() == y);
	});
}

auto ChunkSet::getIter(int x, int y) const
{
	return std::find_if(this->chunks.begin(), this->chunks.end(),
		[x, y](const std::unique_ptr<Chunk> &chunk)
	{
		return (chunk->getX() == x) && (chunk->getY() == y);
	});
}

Chunk *ChunkSet::getPtr(int x, int y) const
{
	const auto iter = this->getIter(x, y);
	return (iter != this->chunks.end()) ? iter->get() : nullptr;
}

Chunk *ChunkSet::getInternal(int x, int y) const
{
	Chunk *ptr = this->getPtr(x, y);

	if (ptr != nullptr)
	{
		return ptr;
	}
	else
	{
		// If outside the chunk set and it is set to wrap, then get the wrapped chunk.
		if (this->wrap)
		{
			int newX, newY;
			this->getWrappedCoords(x, y, newX, newY);
			return this->getPtr(newX, newY);
		}
		else
		{
			// No chunk.
			return nullptr;
		}
	}
}

Chunk *ChunkSet::getRelativeInternal(int srcX, int srcY, int srcVoxelX, int srcVoxelZ) const
{
	// Find how many chunks away it is. Remember voxel X is north-south, Z is east-west.
	const int chunkDiffX = ((srcVoxelZ >= 0) ?
		srcVoxelZ : ((srcVoxelZ - Chunk::DEPTH) + 1)) / Chunk::DEPTH;
	const int chunkDiffY = ((srcVoxelX >= 0) ?
		srcVoxelX : ((srcVoxelX - Chunk::WIDTH) + 1)) / Chunk::WIDTH;
	const int dstX = srcX + chunkDiffX;
	const int dstY = srcY - chunkDiffY;

	return this->getInternal(dstX, dstY);
}

int ChunkSet::getWidth() const
{
	// Find min and max X chunk coordinates.
	const auto iters = std::minmax_element(this->chunks.begin(), this->chunks.end(),
		[](const std::unique_ptr<Chunk> &a, const std::unique_ptr<Chunk> &b)
	{
		return a->getX() < b->getX();
	});

	if (iters.first != this->chunks.end())
	{
		const int minX = (*iters.first)->getX();
		const int maxX = (*iters.second)->getX();
		return maxX - minX;
	}
	else
	{
		return 0;
	}
}

int ChunkSet::getHeight() const
{
	// Find min and max Y chunk coordinates.
	const auto iters = std::minmax_element(this->chunks.begin(), this->chunks.end(),
		[](const std::unique_ptr<Chunk> &a, const std::unique_ptr<Chunk> &b)
	{
		return a->getY() < b->getY();
	});

	if (iters.first != this->chunks.end())
	{
		const int minY = (*iters.first)->getY();
		const int maxY = (*iters.second)->getY();
		return maxY - minY;
	}
	else
	{
		return 0;
	}
}

void ChunkSet::getWrappedCoords(int x, int y, int &dstX, int &dstY) const
{
	// @todo: might need to revise this so it tiles like a checkerboard properly.
	// Don't need to worry about wrapping coordinates in a chunk set that doesn't include (0, 0),
	// because the only one that doesn't is the wilderness and it doesn't wrap.
	dstX = x % this->getWidth();
	dstY = y % this->getHeight();
}

int ChunkSet::getCount() const
{
	return static_cast<int>(this->chunks.size());
}

Chunk *ChunkSet::get(int x, int y)
{
	return this->getInternal(x, y);
}

const Chunk *ChunkSet::get(int x, int y) const
{
	return this->getInternal(x, y);
}

Chunk *ChunkSet::get(int index)
{
	return (index < this->getCount()) ? this->chunks[index].get() : nullptr;
}

const Chunk *ChunkSet::get(int index) const
{
	return (index < this->getCount()) ? this->chunks[index].get() : nullptr;
}

Chunk *ChunkSet::getRelative(int srcX, int srcY, int srcVoxelX, int srcVoxelZ)
{
	return this->getRelativeInternal(srcX, srcY, srcVoxelX, srcVoxelZ);
}

const Chunk *ChunkSet::getRelative(int srcX, int srcY, int srcVoxelX, int srcVoxelZ) const
{
	return this->getRelativeInternal(srcX, srcY, srcVoxelX, srcVoxelZ);
}

Chunk &ChunkSet::insert(int x, int y)
{
	auto makeChunk = [this](int x, int y) -> std::unique_ptr<Chunk>
	{
		if (this->exterior)
		{
			return std::make_unique<ExteriorChunk>(x, y);
		}
		else
		{
			return std::make_unique<InteriorChunk>(x, y);
		}
	};

	const auto iter = this->getIter(x, y);

	// Add if it doesn't exist, overwrite if it does.
	const bool exists = iter != this->chunks.end();

	if (exists)
	{
		*iter = makeChunk(x, y);
		return *iter->get();
	}
	else
	{
		this->chunks.push_back(makeChunk(x, y));
		return *this->chunks.back().get();
	}
}

void ChunkSet::remove(int x, int y)
{
	const auto iter = this->getIter(x, y);

	// Remove if the chunk exists.
	const bool exists = iter != this->chunks.end();

	if (exists)
	{
		this->chunks.erase(iter);
	}
}
