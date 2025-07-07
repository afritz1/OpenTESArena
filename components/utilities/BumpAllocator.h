#ifndef BUMP_ALLOCATOR_H
#define BUMP_ALLOCATOR_H

#include <cstddef>
#include <type_traits>

#include "Buffer.h"
#include "Bytes.h"
#include "Span.h"

// Simple allocate-only buffer for POD types that "bumps" the next address to allocate at. Very cheap to destroy.
class BumpAllocator
{
private:
	Buffer<std::byte> data;
	int index;

	template<typename T>
	static int getByteCount(int count)
	{
		return static_cast<int>(count * sizeof(T));
	}

	// Gets the number of bytes needed to push an allocation to the next valid alignment for T.
	template<typename T>
	int getBytesToNextAlignment() const
	{
		DebugAssert(this->data.isValid());
		return Bytes::getBytesToNextAlignment(reinterpret_cast<uintptr_t>(this->data.begin() + this->index), alignof(T));
	}

	template<typename T>
	int getCombinedByteCount(int count) const
	{
		const int byteCount = BumpAllocator::getByteCount<T>(count);
		const int alignmentBytes = this->getBytesToNextAlignment<T>();
		return byteCount + alignmentBytes;
	}
public:
	BumpAllocator(int byteCount)
	{
		this->init(byteCount);
	}

	BumpAllocator()
	{
		this->index = 0;
	}

	void init(int byteCount)
	{
		this->data.init(byteCount);
		this->index = 0;
	}

	int getByteSize() const
	{
		return this->data.getCount();
	}

	template<typename T>
	bool canAlloc(int count) const
	{
		const int byteCount = this->getCombinedByteCount<T>(count);
		return (this->index + byteCount) <= this->data.getCount();
	}

	template<typename T>
	bool canAlloc() const
	{
		return this->canAlloc<T>(1);
	}

	template<typename T>
	Span<T> alloc(int count, const T &defaultValue)
	{
		static_assert(std::is_trivial_v<T>);
		DebugAssert(count >= 0);
		DebugAssert(this->canAlloc<T>(count));

		this->index += this->getBytesToNextAlignment<T>();
		T *ptr = reinterpret_cast<T*>(this->data.begin() + this->index);
		for (int i = 0; i < count; i++)
		{
			*(ptr + i) = defaultValue;
		}

		this->index += BumpAllocator::getByteCount<T>(count);
		return Span<T>(ptr, count);
	}

	template<typename T>
	Span<T> alloc(int count)
	{
		return this->alloc(count, T());
	}

	template<typename T>
	T *alloc()
	{
		Span<T> span = this->alloc(1, T());
		return span.begin();
	}

	void clear()
	{
		this->index = 0;
	}
};

#endif
