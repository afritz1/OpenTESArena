#ifndef BUFFER_VIEW_READ_ONLY_H
#define BUFFER_VIEW_READ_ONLY_H

#include <algorithm>
#include <type_traits>

#include "../debug/Debug.h"

// Simple non-owning view over a 1D range of data. Useful when separating a container from the usage
// of its data.

// Data can be null. Only need assertions on things that reach into the buffer itself.

template <typename T>
class BufferViewReadOnly
{
private:
	const T *data;
	const int count;
public:
	BufferViewReadOnly() : BufferViewReadOnly(nullptr, 0)
	{
		this->reset();
	}

	// View across a subset of a range of data. Provided for bounds-checking the view range
	// inside a full range (data, data + count) at initialization.
	BufferViewReadOnly(const T *data, int count, int viewOffset, int viewCount) : data(data + viewOffset), count(viewCount)
	{
		DebugAssert(count >= 0);
		DebugAssert(viewOffset >= 0);
		DebugAssert(viewCount >= 0);
		DebugAssert((viewOffset + viewCount) <= count);
	}

	BufferViewReadOnly(T *data, int count, int viewOffset, int viewCount) : data(data + viewOffset), count(viewCount)
	{
	}

	BufferViewReadOnly(const T *data, int count) : BufferViewReadOnly(data, count, 0, count)
	{
	}

	BufferViewReadOnly(T *data, int count) : BufferViewReadOnly(data, count, 0, count)
	{
	}

	bool isValid() const
	{
		return this->data != nullptr;
	}

	const T *get() const
	{
		return this->data;
	}

	const T &get(int index) const
	{
		DebugAssert(this->isValid());
		DebugAssert(index >= 0);
		DebugAssert(index < this->count);
		return this->data[index];
	}

	const T *end() const
	{
		return (this->data != nullptr) ? (this->data + this->count) : nullptr;
	}

	int getCount() const
	{
		return this->count;
	}
};

#endif
