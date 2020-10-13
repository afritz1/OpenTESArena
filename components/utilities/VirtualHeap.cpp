#include <algorithm>
#include <iterator>

#include "VirtualHeap.h"

#include "../debug/Debug.h"

VirtualHeap::Block::Block(Offset offset, Size size)
{
	this->offset = offset;
	this->size = size;
}

bool VirtualHeap::Block::isInfinite() const
{
	return this->size == INFINITE_BLOCK_SIZE;
}

void VirtualHeap::Block::combineLeft(const Block &block)
{
	DebugAssert(!block.isInfinite());
	DebugAssert((block.offset + block.size) == this->offset);

	this->offset -= block.size;
	if (!this->isInfinite())
	{
		this->size += block.size;
	}
}

void VirtualHeap::Block::combineRight(const Block &block)
{
	DebugAssert(!this->isInfinite());
	DebugAssert((this->offset + this->size) == block.offset);

	if (block.isInfinite())
	{
		this->size = INFINITE_BLOCK_SIZE;
	}
	else
	{
		this->size += block.size;
	}
}

VirtualHeap::VirtualHeap()
{
	this->nextHandle = 0;

	// Allocate one infinite free block to start.
	Block block(0, INFINITE_BLOCK_SIZE);
	this->freeBlocks.push_back(block);
}

VirtualHeap::Handle VirtualHeap::getNextHandle()
{
	Handle handle;
	if (!this->freedHandles.empty())
	{
		handle = this->freedHandles.back();
		this->freedHandles.pop_back();
	}
	else
	{
		handle = this->nextHandle;
		this->nextHandle++;
	}

	return handle;
}

bool VirtualHeap::tryGetBlock(Handle handle, const Block **outBlock) const
{
	const auto iter = this->usedBlocks.find(handle);
	if (iter != this->usedBlocks.end())
	{
		*outBlock = &iter->second;
		return true;
	}
	else
	{
		return false;
	}
}

VirtualHeap::Handle VirtualHeap::alloc(Block::Size size)
{
	if (size <= 0)
	{
		DebugLogWarning("Allocation size (" + std::to_string(size) + ") must be positive.");
		return INVALID_HANDLE;
	}

	// Adds a new used block and returns its unique handle.
	auto addUsedBlock = [this](VirtualHeap::Block::Offset offset, VirtualHeap::Block::Size size)
	{
		const Handle handle = this->getNextHandle();
		this->usedBlocks.emplace(handle, Block(offset, size));
		return handle;
	};

	// Find a suitable free block via first-fit.
	VirtualHeap::Handle handle = INVALID_HANDLE;
	for (auto freeBlock = this->freeBlocks.begin(); freeBlock != this->freeBlocks.end(); ++freeBlock)
	{
		if (freeBlock->isInfinite())
		{
			// Trivial case; allocate new then push the infinite block's offset back.
			handle = addUsedBlock(freeBlock->offset, size);
			freeBlock->offset += size;
			break;
		}
		else if (freeBlock->size > size)
		{
			// Partial free block allocation.
			handle = addUsedBlock(freeBlock->offset, size);
			freeBlock->offset += size;
			freeBlock->size -= size;
			break;
		}
		else if (freeBlock->size == size)
		{
			// Full free block allocation.
			handle = addUsedBlock(freeBlock->offset, size);
			this->freeBlocks.erase(freeBlock);
			break;
		}
	}

	return handle;
}

void VirtualHeap::free(Handle handle)
{
	if (handle == INVALID_HANDLE)
	{
		DebugLogWarning("Tried freeing invalid handle \"" + std::to_string(handle) + "\".");
		return;
	}

	const auto usedBlockIter = this->usedBlocks.find(handle);
	if (usedBlockIter == this->usedBlocks.end())
	{
		DebugLogWarning("No block to free for handle \"" + std::to_string(handle) + "\".");
		return;
	}

	const Block &usedBlock = usedBlockIter->second;

	// Find the first free block after the used block. There will always be at least one free
	// block somewhere to the right.
	const auto nextFreeIter = std::find_if(this->freeBlocks.begin(), this->freeBlocks.end(),
		[&usedBlock](const Block &freeBlock)
	{
		return freeBlock.offset > usedBlock.offset;
	});

	DebugAssert(nextFreeIter != this->freeBlocks.end());

	// When coalescing, check if there are adjacent free blocks to the left and right
	// of the allocated block.
	const bool adjToFreeRight = (usedBlock.offset + usedBlock.size) == nextFreeIter->offset;
	const bool adjToFreeLeft = [this, &usedBlock, &nextFreeIter]()
	{
		if (nextFreeIter != this->freeBlocks.begin())
		{
			const auto prevFreeIter = std::prev(nextFreeIter);
			DebugAssert(!prevFreeIter->isInfinite());
			return (prevFreeIter->offset + prevFreeIter->size) == usedBlock.offset;
		}
		else return false;
	}();

	if (adjToFreeLeft && adjToFreeRight)
	{
		// Free blocks on the left and right. Merge the used block and right free block into
		// the left free block.
		const auto prevFreeIter = std::prev(nextFreeIter);
		prevFreeIter->combineRight(usedBlock);
		prevFreeIter->combineRight(*nextFreeIter);
		this->freeBlocks.erase(nextFreeIter);
	}
	else if (adjToFreeLeft && !adjToFreeRight)
	{
		// Free block on the left. Merge the used block into it.
		const auto prevFreeIter = std::prev(nextFreeIter);
		prevFreeIter->combineRight(usedBlock);
	}
	else if (!adjToFreeLeft && adjToFreeRight)
	{
		// Free block on the right. Merge the used block into it.
		nextFreeIter->combineLeft(usedBlock);
	}
	else
	{
		// Not adjacent to any free blocks. Create a new free block.
		this->freeBlocks.emplace(nextFreeIter, Block(usedBlock.offset, usedBlock.size));
	}

	// Remove the used block and release its unique handle.
	this->usedBlocks.erase(handle);
	this->freedHandles.push_back(handle);
}
