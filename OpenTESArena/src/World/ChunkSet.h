#ifndef CHUNK_SET_H
#define CHUNK_SET_H

#include <vector>

#include "Chunk.h"

// Dynamic group of all active chunks. Chunks are added and removed by a caller as needed.
// This only stores the voxels in each chunk, not the entities.

// @todo: maybe sort chunks from top-leftmost to bottom-rightmost, like text? Instead of
// being unsorted? Might not make a difference.

template <typename T>
class ChunkSet
{
private:
	std::vector<T> chunks;

	// Convenience function for getting an iterator to a chunk if it exists, or the end of
	// the chunks list if it doesn't.
	auto getIter(int x, int y, const std::vector<T> &chunks);
	auto getIter(int x, int y, const std::vector<T> &chunks) const;
public:
	// Returns number of chunks in the set.
	int getCount() const;

	// Returns pointer to the requested chunk if it exists, or null if it doesn't.
	// The player's chunk should be treated as the current origin chunk that all others
	// are relative to.
	T *get(int x, int y);
	const T *get(int x, int y) const;

	// Functions for iterating over all chunks in the set. Returns null once the end is reached.
	T *get(int index);
	const T *get(int index) const;

	// Adds a chunk at the given coordinate, overwriting any existing one.
	void set(int x, int y, const T &chunk);
	void set(int x, int y, T &&chunk);

	// Removes a chunk at the given coordinate if it exists.
	void remove(int x, int y);
};

// Template instantiations at end of .cpp file.
using InteriorChunkSet = ChunkSet<InteriorChunk>;
using ExteriorChunkSet = ChunkSet<ExteriorChunk>;

#endif
