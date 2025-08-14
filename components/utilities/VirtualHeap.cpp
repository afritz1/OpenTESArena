#include <algorithm>
#include <iterator>

#include "VirtualHeap.h"

#include "../debug/Debug.h"

VirtualHeapBlock::VirtualHeapBlock(VirtualHeapBlockOffset offset, VirtualHeapBlockSize size)
{
	this->offset = offset;
	this->size = size;
}

bool VirtualHeapBlock::isInfinite() const
{
	return this->size == VIRTUAL_HEAP_INFINITE_BLOCK_SIZE;
}

void VirtualHeapBlock::combineLeft(const VirtualHeapBlock &block)
{
	DebugAssert(!block.isInfinite());
	DebugAssert((block.offset + block.size) == this->offset);

	this->offset -= block.size;
	if (!this->isInfinite())
	{
		this->size += block.size;
	}
}

void VirtualHeapBlock::combineRight(const VirtualHeapBlock &block)
{
	DebugAssert(!this->isInfinite());
	DebugAssert((this->offset + this->size) == block.offset);

	if (block.isInfinite())
	{
		this->size = VIRTUAL_HEAP_INFINITE_BLOCK_SIZE;
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
	VirtualHeapBlock block(0, VIRTUAL_HEAP_INFINITE_BLOCK_SIZE);
	this->freeBlocks.push_back(block);
}

VirtualHeapHandle VirtualHeap::getNextHandle()
{
	VirtualHeapHandle handle;
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

bool VirtualHeap::tryGetBlock(VirtualHeapHandle handle, const VirtualHeapBlock **outBlock) const
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

VirtualHeapHandle VirtualHeap::alloc(VirtualHeapBlockSize size)
{
	if (size <= 0)
	{
		DebugLogWarningFormat("Allocation size %d must be positive.", size);
		return INVALID_HANDLE;
	}

	// Adds a new used block and returns its unique handle.
	auto addUsedBlock = [this](VirtualHeapBlockOffset offset, VirtualHeapBlockSize size)
	{
		const VirtualHeapHandle handle = this->getNextHandle();
		this->usedBlocks.emplace(handle, VirtualHeapBlock(offset, size));
		return handle;
	};

	// Find a suitable free block via first-fit.
	VirtualHeapHandle handle = INVALID_HANDLE;
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

void VirtualHeap::free(VirtualHeapHandle handle)
{
	if (handle == INVALID_HANDLE)
	{
		DebugLogWarningFormat("Tried freeing invalid handle %d.", handle);
		return;
	}

	const auto usedBlockIter = this->usedBlocks.find(handle);
	if (usedBlockIter == this->usedBlocks.end())
	{
		DebugLogWarningFormat("No block to free for handle %d.", handle);
		return;
	}

	const VirtualHeapBlock &usedBlock = usedBlockIter->second;

	// Find the first free block after the used block. There will always be at least one free
	// block somewhere to the right.
	const auto nextFreeIter = std::find_if(this->freeBlocks.begin(), this->freeBlocks.end(),
		[&usedBlock](const VirtualHeapBlock &freeBlock)
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
		this->freeBlocks.emplace(nextFreeIter, VirtualHeapBlock(usedBlock.offset, usedBlock.size));
	}

	// Remove the used block and release its unique handle.
	this->usedBlocks.erase(handle);
	this->freedHandles.push_back(handle);
}
