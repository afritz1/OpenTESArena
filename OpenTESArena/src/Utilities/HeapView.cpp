#include <limits>

#include "Debug.h"
#include "HeapView.h"

HeapView::HeapView()
{
	// Allocate one "infinite" free block.
	Block block(0, std::numeric_limits<size_t>::max());
	this->blocks.push_back(block);
}

size_t HeapView::allocate(size_t size)
{
	// Allocation request must be at least 1 byte.
	DebugAssert(size > 0, "Allocation size must be positive.");

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

void HeapView::deallocate(size_t offset)
{
	// See if an allocation exists at the given offset.
	const auto sizeIter = this->sizes.find(offset);
	DebugAssert(sizeIter != this->sizes.end(),
		"Invalid index for deallocation (" + std::to_string(offset) + ").");

	// Get size of block at offset.
	const size_t size = sizeIter->second;

	// Get an iterator to the nearest free block after the allocated block.
	// It is guaranteed that there will always be at least one free block somewhere
	// to the right of an allocation.
	const auto nextFreeIter = [this, offset]()
	{
		auto it = this->blocks.begin();
		while (it->offset <= offset)
		{
			++it;
		}

		return it;
	}();

	// When coalescing, check if there are adjacent free blocks to the left and right
	// of the allocated block.
	const bool adjToFreeRight = (offset + size) == nextFreeIter->offset;
	const bool adjToFreeLeft = [this, &nextFreeIter, offset]()
	{
		if (nextFreeIter != this->blocks.begin())
		{
			const auto prevFreeIter = std::prev(nextFreeIter);
			return (prevFreeIter->offset + prevFreeIter->size) == offset;
		}
		else return false;
	}();

	if (adjToFreeLeft && adjToFreeRight)
	{
		// Free blocks on the left and right.
		const auto prevFreeIter = std::prev(nextFreeIter);
		prevFreeIter->size += size + nextFreeIter->size;

		// Combine the left and right free blocks together.
		this->blocks.erase(nextFreeIter);
	}
	else if (adjToFreeLeft && !adjToFreeRight)
	{
		// Free block on the left.
		const auto prevFreeIter = std::prev(nextFreeIter);
		prevFreeIter->size += size;
	}
	else if (!adjToFreeLeft && adjToFreeRight)
	{
		// Free block on the right.
		nextFreeIter->offset -= size;
		nextFreeIter->size += size;
	}
	else
	{
		// Not adjacent to any free blocks. Create a new free block.
		this->blocks.insert(nextFreeIter, Block(offset, size));
	}

	// Remove the allocation's header.
	this->sizes.erase(sizeIter);
}
