#ifndef BUFFER_VIEW_H
#define BUFFER_VIEW_H

#include <algorithm>
#include <type_traits>

#include "../debug/Debug.h"

// Simple non-owning view over a 1D range of data. Useful when separating a container from the usage
// of its data.

// Data can be null. Only need assertions on things that reach into the buffer itself.

template <typename T>
class BufferView
{
private:
	T *data;
	int count;
public:
	BufferView()
	{
		this->reset();
	}

	// View across a subset of a range of data. Provided for bounds-checking the view range
	// inside a full range (data, data + count) at initialization.
	BufferView(T *data, int count, int viewOffset, int viewCount)
	{
		this->init(data, count, viewOffset, viewCount);
	}

	// View across a range of data.
	BufferView(T *data, int count)
	{
		this->init(data, count);
	}

	void init(T *data, int count, int viewOffset, int viewCount)
	{
		DebugAssert(count >= 0);
		DebugAssert(viewOffset >= 0);
		DebugAssert(viewCount >= 0);
		DebugAssert((viewOffset + viewCount) <= count);
		this->data = data + viewOffset;
		this->count = viewCount;
	}

	void init(T *data, int count)
	{
		this->init(data, count, 0, count);
	}

	bool isValid() const
	{
		return this->data != nullptr;
	}

	T *get()
	{		
		return this->data;
	}

	const T *get() const
	{
		return this->data;
	}

	T &get(int index)
	{
		DebugAssert(this->isValid());
		DebugAssert(index >= 0);
		DebugAssert(index < this->count);
		return this->data[index];
	}

	const T &get(int index) const
	{
		DebugAssert(this->isValid());
		DebugAssert(index >= 0);
		DebugAssert(index < this->count);
		return this->data[index];
	}

	T *end()
	{
		return (this->data != nullptr) ? (this->data + this->count) : nullptr;
	}

	const T *end() const
	{
		return (this->data != nullptr) ? (this->data + this->count) : nullptr;
	}

	int getCount() const
	{
		return this->count;
	}

	void set(int index, const T &value)
	{
		static_assert(!std::is_const_v<T>, "Cannot change const data.");

		DebugAssert(this->isValid());
		DebugAssert(index >= 0);
		DebugAssert(index < this->count);
		this->data[index] = value;
	}

	void set(int index, T &&value)
	{
		static_assert(!std::is_const_v<T>, "Cannot change const data.");

		DebugAssert(this->isValid());
		DebugAssert(index >= 0);
		DebugAssert(index < this->count);
		this->data[index] = std::move(value);
	}

	void fill(const T &value)
	{
		static_assert(!std::is_const_v<T>, "Cannot change const data.");

		DebugAssert(this->isValid());
		std::fill(this->data, this->end(), value);
	}

	void reset()
	{
		this->data = nullptr;
		this->count = 0;
	}
};

#endif
