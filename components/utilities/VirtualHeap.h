#ifndef VIRTUAL_HEAP_H
#define VIRTUAL_HEAP_H

#include <cstdint>
#include <deque>
#include <type_traits>
#include <unordered_map>
#include <vector>

using VirtualHeapHandle = int;
using VirtualHeapBlockOffset = uint64_t;
using VirtualHeapBlockSize = int;

static constexpr VirtualHeapBlockSize VIRTUAL_HEAP_INFINITE_BLOCK_SIZE = -1;

struct VirtualHeapBlock
{
	VirtualHeapBlockOffset offset;
	VirtualHeapBlockSize size;

	VirtualHeapBlock(VirtualHeapBlockOffset offset, VirtualHeapBlockSize size);

	// Whether the block has infinite size.
	bool isInfinite() const;

	// Expansion functions for acquiring an adjacent block's space.
	void combineLeft(const VirtualHeapBlock &block);
	void combineRight(const VirtualHeapBlock &block);
};

// An infinite-sized heap that tells where to make allocations in an actual buffer. Only the
// positions and sizes of imaginary blocks are stored.
class VirtualHeap
{
private:
	static_assert(std::is_signed_v<VirtualHeapBlockSize>);

	std::deque<VirtualHeapBlock> freeBlocks;
	std::unordered_map<VirtualHeapHandle, VirtualHeapBlock> usedBlocks;
	std::vector<VirtualHeapHandle> freedHandles;
	VirtualHeapHandle nextHandle;

	VirtualHeapHandle getNextHandle();
public:
	static constexpr VirtualHeapHandle INVALID_HANDLE = -1;

	VirtualHeap();

	// Tries to get the virtual heap block associated with the given handle.
	bool tryGetBlock(VirtualHeapHandle handle, const VirtualHeapBlock **outBlock) const;

	// Allocates a virtual block with the given size and returns a handle to it, or an
	// invalid handle if it couldn't be allocated. This block might not fit in the caller's
	// buffer, in which case they must either re-allocate or try something else.
	VirtualHeapHandle alloc(VirtualHeapBlockSize size);

	// Frees the given handle's virtual block.
	void free(VirtualHeapHandle handle);
};

#endif
