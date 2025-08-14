#include "Bytes.h"
#include "Heap.h"
#include "Span.h"
#include "../debug/Debug.h"

namespace
{
	int GetTotalByteCount(Span<const HeapBlock> blocks)
	{
		int count = 0;
		for (const HeapBlock &block : blocks)
		{
			count += block.byteCount;
		}

		return count;
	}
}

HeapBlock::HeapBlock()
{
	this->offset = 0;
	this->byteCount = -1;
}

bool HeapBlock::isValid() const
{
	return this->byteCount >= 0;
}

HeapAllocator::HeapAllocator()
{
	this->baseAddress = 0;
}

void HeapAllocator::init(uintptr_t baseAddress, int byteCount)
{
	DebugAssert(baseAddress > 0);
	DebugAssert(byteCount >= 0);
	this->baseAddress = baseAddress;

	HeapBlock block;
	block.offset = Bytes::getBytesToNextAlignment(baseAddress, DefaultAlignment);
	block.byteCount = byteCount - block.offset;
	this->freeBlocks.emplace_back(std::move(block));
}

int HeapAllocator::getFreeBytes() const
{
	return GetTotalByteCount(this->freeBlocks);
}

int HeapAllocator::getUsedBytes() const
{
	return GetTotalByteCount(this->usedBlocks);
}

HeapBlock HeapAllocator::alloc(int byteCount, int alignment)
{
	if ((byteCount <= 0) || (alignment < 1))
	{
		return HeapBlock();
	}

	HeapBlock foundFreeBlock;
	for (int i = static_cast<int>(this->freeBlocks.size()) - 1; i >= 0; i--)
	{
		HeapBlock &freeBlock = this->freeBlocks[i];
		const int bytesToNextAlignment = Bytes::getBytesToNextAlignment(this->baseAddress + freeBlock.offset, alignment);
		const int requiredBytes = byteCount + bytesToNextAlignment;
		if (freeBlock.byteCount >= requiredBytes)
		{
			foundFreeBlock.offset = freeBlock.offset + bytesToNextAlignment;
			foundFreeBlock.byteCount = byteCount;

			const int oldFreeBlockByteCount = freeBlock.byteCount;

			// The free block may have split in two if its alignment wasn't enough.
			if (bytesToNextAlignment > 0)
			{
				freeBlock.byteCount = bytesToNextAlignment;

				if (oldFreeBlockByteCount > requiredBytes)
				{
					HeapBlock newFreeBlock;
					newFreeBlock.offset = foundFreeBlock.offset + foundFreeBlock.byteCount;
					newFreeBlock.byteCount = oldFreeBlockByteCount - requiredBytes;
					this->freeBlocks.insert(this->freeBlocks.begin() + i + 1, std::move(newFreeBlock));
				}
			}
			else
			{
				if (oldFreeBlockByteCount > requiredBytes)
				{
					freeBlock.offset = foundFreeBlock.offset + foundFreeBlock.byteCount;
					freeBlock.byteCount = oldFreeBlockByteCount - byteCount;
				}
				else
				{
					this->freeBlocks.erase(this->freeBlocks.begin() + i);
				}
			}

			break;
		}
	}

	if (!foundFreeBlock.isValid())
	{
		DebugLogErrorFormat("Couldn't find free block to fulfill %d bytes and alignment %d.", byteCount, alignment);
		return HeapBlock();
	}

	// Insert sorted into used blocks.
	int usedBlocksInsertIndex = 0;
	for (int i = 0; i < static_cast<int>(this->usedBlocks.size()); i++)
	{
		if (this->usedBlocks[i].offset > foundFreeBlock.offset)
		{
			break;
		}

		usedBlocksInsertIndex++;
	}

	this->usedBlocks.insert(this->usedBlocks.begin() + usedBlocksInsertIndex, foundFreeBlock);

	return foundFreeBlock;
}

void HeapAllocator::free(HeapBlock block)
{
	if (!block.isValid())
	{
		DebugLogWarningFormat("Tried to free invalid block with offset %d and bytes %d.", block.offset, block.byteCount);
		return;
	}

	int usedBlockIndex = -1;
	for (int i = 0; i < static_cast<int>(this->usedBlocks.size()); i++)
	{
		const HeapBlock usedBlock = this->usedBlocks[i];
		if ((usedBlock.offset == block.offset) && (usedBlock.byteCount == block.byteCount))
		{
			usedBlockIndex = i;
			break;
		}
	}

	if (usedBlockIndex < 0)
	{
		DebugLogWarningFormat("Couldn't find block to free at offset %d with bytes %d.", block.offset, block.byteCount);
		return;
	}

	int leftAdjacentFreeBlockIndex = -1;
	int rightAdjacentFreeBlockIndex = -1;
	for (int i = 0; i < static_cast<int>(this->freeBlocks.size()); i++)
	{
		const HeapBlock freeBlock = this->freeBlocks[i];

		const bool isLeftAdjacent = (freeBlock.offset + freeBlock.byteCount) == block.offset;
		const bool isRightAdjacent = (block.offset + block.byteCount) == freeBlock.offset;
		const bool isBeyondRightAdjacent = (block.offset + block.byteCount) < freeBlock.offset;

		if (isBeyondRightAdjacent)
		{
			break;
		}
		else if (isLeftAdjacent)
		{
			leftAdjacentFreeBlockIndex = i;
		}
		else if (isRightAdjacent)
		{
			rightAdjacentFreeBlockIndex = i;
		}

		if (leftAdjacentFreeBlockIndex >= 0 && rightAdjacentFreeBlockIndex >= 0)
		{
			break;
		}
	}

	if (leftAdjacentFreeBlockIndex >= 0)
	{
		HeapBlock &leftFreeBlock = this->freeBlocks[leftAdjacentFreeBlockIndex];
		leftFreeBlock.byteCount += block.byteCount;

		if (rightAdjacentFreeBlockIndex >= 0)
		{
			const HeapBlock rightFreeBlock = this->freeBlocks[rightAdjacentFreeBlockIndex];
			leftFreeBlock.byteCount += rightFreeBlock.byteCount;
			this->freeBlocks.erase(this->freeBlocks.begin() + rightAdjacentFreeBlockIndex);
		}
	}
	else if (rightAdjacentFreeBlockIndex >= 0)
	{
		HeapBlock &rightFreeBlock = this->freeBlocks[rightAdjacentFreeBlockIndex];
		rightFreeBlock.offset -= block.byteCount;
		rightFreeBlock.byteCount += block.byteCount;
	}
	else
	{
		// No adjacent free blocks, make one at the used block index.
		int freeBlockInsertIndex = 0;
		for (int i = 0; i < static_cast<int>(this->freeBlocks.size()); i++)
		{
			if (this->freeBlocks[i].offset > block.offset)
			{
				break;
			}

			freeBlockInsertIndex++;
		}
		
		HeapBlock newFreeBlock;
		newFreeBlock.offset = block.offset;
		newFreeBlock.byteCount = block.byteCount;
		this->freeBlocks.insert(this->freeBlocks.begin() + freeBlockInsertIndex, std::move(newFreeBlock));
	}

	this->usedBlocks.erase(this->usedBlocks.begin() + usedBlockIndex);
}

void HeapAllocator::clear()
{
	this->baseAddress = 0;
	this->freeBlocks.clear();
	this->usedBlocks.clear();
}
