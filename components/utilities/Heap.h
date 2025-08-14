#ifndef HEAP_H
#define HEAP_H

#include <cstdint>
#include <vector>

struct HeapBlock
{
	int offset;
	int byteCount;

	HeapBlock();

	bool isValid() const;
};

// An allocator for a fixed size buffer owned elsewhere.
class HeapAllocator
{
private:
	uintptr_t baseAddress;
	std::vector<HeapBlock> freeBlocks;
	std::vector<HeapBlock> usedBlocks;
public:
	static constexpr int DefaultAlignment = 8;

	HeapAllocator();

	void init(uintptr_t baseAddress, int byteCount);

	int getFreeBytes() const;
	int getUsedBytes() const;

	HeapBlock alloc(int byteCount, int alignment = DefaultAlignment);

	void free(HeapBlock block);

	void clear();
};

#endif
