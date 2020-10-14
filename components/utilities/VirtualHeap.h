#ifndef VIRTUAL_HEAP_H
#define VIRTUAL_HEAP_H

#include <deque>
#include <type_traits>
#include <unordered_map>
#include <vector>

// An infinite-sized heap that tells where to make allocations in an actual buffer. Only the
// positions and sizes of imaginary blocks are stored.

class VirtualHeap
{
public:
	using Handle = int;

	struct Block
	{
		using Offset = uint64_t;
		using Size = int;

		Offset offset;
		Size size;

		Block(Offset offset, Size size);

		// Whether the block has infinite size.
		bool isInfinite() const;

		// Expansion functions for acquiring an adjacent block's space.
		void combineLeft(const Block &block);
		void combineRight(const Block &block);
	};
private:
	static_assert(std::is_signed_v<Block::Size>);
	static constexpr Block::Size INFINITE_BLOCK_SIZE = -1;

	std::deque<Block> freeBlocks;
	std::unordered_map<Handle, Block> usedBlocks;
	std::vector<Handle> freedHandles;
	Handle nextHandle;

	Handle getNextHandle();
public:
	static constexpr Handle INVALID_HANDLE = -1;

	VirtualHeap();

	// Tries to get the virtual heap block associated with the given handle.
	bool tryGetBlock(Handle handle, const Block **outBlock) const;

	// Allocates a virtual block with the given size and returns a handle to it, or an
	// invalid handle if it couldn't be allocated. This block might not fit in the caller's
	// buffer, in which case they must either re-allocate or try something else.
	Handle alloc(Block::Size size);

	// Frees the given handle's virtual block.
	void free(Handle handle);
};

#endif
