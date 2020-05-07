#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <cstddef>
#include <type_traits>

#include "Buffer.h"
#include "BufferView.h"

// Simple scratch allocator for POD types. Very fast destruction, intended for clearing
// frequently without worrying about heap fragmentation.

class ScratchAllocator
{
private:
	Buffer<std::byte> data;
	int index;

	template <typename T>
	static int getByteCount(int count)
	{
		return static_cast<int>(count * sizeof(T));
	}

	// Gets the number of bytes needed to push an allocation to the next valid alignment for T.
	template <typename T>
	int getAlignmentByteCount() const
	{
		DebugAssert(this->data.isValid());
		constexpr size_t alignment = alignof(T);
		const size_t curAddress = reinterpret_cast<uintptr_t>(this->data.get()) + this->index;
		const size_t modulo = curAddress % alignment;
		return (modulo != 0) ? static_cast<int>(alignment - modulo) : 0;
	}

	template <typename T>
	int getCombinedByteCount(int count) const
	{
		const int byteCount = ScratchAllocator::getByteCount<T>(count);
		const int alignmentBytes = this->getAlignmentByteCount<T>();
		return byteCount + alignmentBytes;
	}
public:
	ScratchAllocator(int byteCount)
	{
		this->init(byteCount);
	}

	ScratchAllocator()
	{
		this->index = 0;
	}

	void init(int byteCount)
	{
		this->data.init(byteCount);
		this->index = 0;
	}

	bool isInited() const
	{
		return this->data.getCount() > 0;
	}

	int getByteSize() const
	{
		return this->data.getCount();
	}

	template <typename T>
	bool canAlloc(int count) const
	{
		if (!this->isInited() || !this->data.isValid())
		{
			return false;
		}

		const int byteCount = this->getCombinedByteCount<T>(count);
		return (index + byteCount) <= this->data.getCount();
	}

	template <typename T>
	BufferView<T> alloc(int count, const T &defaultValue)
	{
		static_assert(std::is_trivial_v<T>);
		DebugAssert(count >= 0);
		DebugAssert(this->canAlloc<T>(count));

		this->index += this->getAlignmentByteCount<T>();
		T *ptr = reinterpret_cast<T*>(this->data.get() + this->index);
		for (int i = 0; i < count; i++)
		{
			*(ptr + i) = defaultValue;
		}

		this->index += ScratchAllocator::getByteCount<T>(count);
		return BufferView<T>(ptr, count);
	}

	template <typename T>
	BufferView<T> alloc(int count)
	{
		return this->alloc(count, T());
	}

	template <typename T>
	T *alloc()
	{
		BufferView<T> bufferView = this->alloc(1, T());
		return bufferView.get();
	}

	void clear()
	{
		this->index = 0;
	}
};

#endif
