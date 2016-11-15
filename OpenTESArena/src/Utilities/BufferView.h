#ifndef BUFFER_VIEW_H
#define BUFFER_VIEW_H

#include <list>
#include <unordered_map>

// A buffer view is an imaginary overlay for a memory buffer, so it can be treated like 
// a linked list of blocks. In other words, it is a guide telling the caller where to 
// allocate a request in an actual buffer. The buffer view itself assumes infinite 
// capacity, so it is the caller's job to make sure their buffer can fit an allocation
// at the suggested byte offset.

class BufferView
{
private:
	struct Block
	{
		size_t offset, size;

		Block(size_t offset, size_t size)
		{
			this->offset = offset;
			this->size = size;
		}
	};

	// Linked list of free blocks.
	std::list<Block> blocks;

	// Mapping of allocated block offsets to their sizes (block headers, basically).
	std::unordered_map<size_t, size_t> sizes;
public:
	BufferView();
	~BufferView();

	// Returns the byte offset for where an allocation of the requested size should 
	// occur. If the returned value points to an offset that would overflow the
	// caller's buffer, their buffer would need to be resized.
	size_t allocate(size_t size);

	// Frees an allocation at the given offset, allowing it to be allocated again. 
	// If no allocation at the offset exists, an error occurs.
	void deallocate(size_t offset);
};

#endif
