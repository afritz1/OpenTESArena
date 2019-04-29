#ifndef CHUNK_SET_H
#define CHUNK_SET_H

#include <memory>
#include <vector>

#include "Chunk.h"

// Dynamic group of all active chunks. Chunks are added and removed by a caller as needed.
// This only stores the voxels in each chunk, not the entities.

// Chunk coordinates are assumed to spatially match a 2D array, with (0, 0) at the top left.

// The voxels outside a level in interiors and cities are obtained by wrapping coordinates.

class ChunkSet
{
private:
	std::vector<std::unique_ptr<Chunk>> chunks;
	bool exterior; // True if exterior, false if interior. Determines chunk allocation.
	bool wrap; // Determines whether out-of-bounds coordinates are wrapped.

	// Convenience function for getting an iterator to a chunk if it exists, or the end of the
	// chunks list if it doesn't.
	auto getIter(int x, int y);
	auto getIter(int x, int y) const;

	// Convenience function for getting a pointer to a chunk if it exists, or null if it doesn't.
	Chunk *getPtr(int x, int y) const;

	// Looks up a chunk and tries again after failure if wrapping is enabled.
	// "Private const function returning non-const" pattern.
	Chunk *getInternal(int x, int y) const;

	// Looks up a chunk that contains the given voxel coordinates relative to the chunk coordinates.
	// "Private const function returning non-const" pattern.
	Chunk *getRelativeInternal(int srcX, int srcY, int srcVoxelX, int srcVoxelZ) const;

	// Gets the dimensions of the chunk set in chunks.
	int getWidth() const;
	int getHeight() const;

	// Gets the wrapped chunk coordinates for the input coordinates. Interiors and cities have
	// their coordinates wrapped when accessing out-of-level voxels.
	void getWrappedCoords(int x, int y, int &dstX, int &dstY) const;
public:
	ChunkSet(bool exterior, bool wrap);

	// Returns number of chunks in the set.
	int getCount() const;

	// Returns pointer to the requested chunk if it exists, or null if it doesn't.
	Chunk *get(int x, int y);
	const Chunk *get(int x, int y) const;

	// Functions for iterating over all chunks in the set. Returns null once the end is reached.
	// Chunks are assumed to be unsorted.
	Chunk *get(int index);
	const Chunk *get(int index) const;

	// Returns pointer to the chunk that contains the relative voxel coordinate, or null if
	// no chunk contains the coordinate.
	Chunk *getRelative(int srcX, int srcY, int srcVoxelX, int srcVoxelZ);
	const Chunk *getRelative(int srcX, int srcY, int srcVoxelX, int srcVoxelZ) const;

	// Adds a chunk, overwriting any existing one at the given coordinates.
	Chunk &insert(int x, int y);

	// Removes a chunk at the given coordinate if it exists.
	void remove(int x, int y);
};

#endif
