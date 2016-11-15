#include <limits>

#include "BufferView.h"

#include "Debug.h"

BufferView::BufferView()
{
	// Allocate one "infinite" free block.
	Block block(0, std::numeric_limits<size_t>::max());
	this->blocks.push_back(block);
}

BufferView::~BufferView()
{

}

size_t BufferView::allocate(size_t size)
{
	// Offset in bytes to a suitable free block.
	size_t offset = 0;

	// Find a spot with enough contiguous free bytes. The buffer view is assumed
	// to have infinite capacity, so it will always eventually find an offset.
	for (auto block = this->blocks.begin(); block != this->blocks.end(); ++block)
	{
		// Does the block have at least the requested bytes (first fit)?
		if (block->size >= size)
		{
			// Set the start of the allocation to where the free block starts.
			offset = block->offset;

			// Add a new block header so the block can be deallocated later.
			this->sizes.insert(std::make_pair(offset, size));

			if (block->size > size)
			{
				// Subtract the allocated space from the free block.
				block->offset += size;
				block->size -= size;
			}
			else
			{
				// The block size equals the requested size. Remove the free block.
				this->blocks.erase(block);
			}

			break;
		}
	}

	// If the sum of the returned offset and the requested size is greater than the 
	// caller's buffer size, then their buffer needs to be resized.
	return offset;
}

void BufferView::deallocate(size_t offset)
{
	// See if an allocation exists at the given offset.
	const auto sizeIter = this->sizes.find(offset);
	Debug::check(sizeIter != this->sizes.end(), "Buffer View",
		"Invalid index for deallocation (" + std::to_string(offset) + ").");

	// Get size of block at offset.
	const size_t size = sizeIter->second;

	// Get an iterator to the nearest free block after the allocated block.
	// It is guaranteed that there will always be at least one free block somewhere
	// to the right of an allocation.
	const auto nextFreeIter = [this, offset]()
	{
		auto it = this->blocks.begin();
		while ((it->offset + it->size) <= offset)
		{
			it++;
		}

		return it;
	}();

	// See if there's a free block immediately to the right.
	const bool rightBlockIsFree = this->sizes.find(offset + size) == this->sizes.end();
	if (rightBlockIsFree)
	{
		// Coalesce with the right block.
		nextFreeIter->offset -= size;
		nextFreeIter->size += size;
	}

	// If the right block is not the first block, see if there's a free block 
	// immediately to the left.
	if (nextFreeIter != this->blocks.begin())
	{
		const auto prevFreeIter = std::prev(nextFreeIter);

		if ((prevFreeIter->offset + prevFreeIter->size) == nextFreeIter->offset)
		{
			// Coalesce the combined (middle + right) block with the left block.
			prevFreeIter->size += nextFreeIter->size;
			this->blocks.erase(nextFreeIter);
		}
		else if ((prevFreeIter->offset + prevFreeIter->size) == offset)
		{
			// Coalesce the newly deallocated block with the left block.
			prevFreeIter->size += size;
		}
		else
		{
			// Insert a new free block before the next free block.
			this->blocks.insert(nextFreeIter, Block(offset, size));
		}
	}
	else if (!rightBlockIsFree)
	{
		// If not adjacent to any free blocks, insert a new free block at the start.
		this->blocks.insert(this->blocks.begin(), Block(offset, size));
	}

	// Remove the allocation's header.
	this->sizes.erase(sizeIter);
}
