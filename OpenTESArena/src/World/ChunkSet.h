#ifndef CHUNK_SET_H
#define CHUNK_SET_H

#include <vector>

#include "Chunk.h"
#include "../Math/Vector2.h"

// Dynamic group of all active chunks. Chunks are added and removed by a caller as needed.
// This only stores the voxels in each chunk, not the entities.

// @todo: maybe sort chunks from top-leftmost to bottom-rightmost, like text? Instead of
// being unsorted? Might not make a difference.

template <typename T>
class ChunkSet
{
private:
	using ChunkList = std::vector<std::pair<Int2, T>>;

	// Chunks with their associated chunk coordinate.
	ChunkList chunks;

	// Convenience function for getting an iterator to a chunk if it exists, or the end of
	// the chunks list if it doesn't.
	auto getIter(const Int2 &point, const ChunkList &chunks);
	auto getIter(const Int2 &point, const ChunkList &chunks) const;
public:
	// Returns number of chunks in the set.
	int getCount() const;

	// Returns pointer to the requested chunk if it exists, or null if it doesn't.
	T *get(const Int2 &point);
	const T *get(const Int2 &point) const;

	// Functions for iterating over all chunks in the set. Returns null once the end is reached.
	std::pair<Int2, T> *getAt(int index);
	const std::pair<Int2, T> *getAt(int index) const;

	// Adds a chunk at the given coordinate, overwriting any existing one.
	void set(const Int2 &point, const T &chunk);
	void set(const Int2 &point, T &&chunk);

	// Removes a chunk at the given coordinate if it exists.
	void remove(const Int2 &point);
};

// Template instantiations at end of .cpp file.
using InteriorChunkSet = ChunkSet<InteriorChunk>;
using ExteriorChunkSet = ChunkSet<ExteriorChunk>;

#endif
