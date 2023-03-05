#ifndef BUFFER_VIEW_H
#define BUFFER_VIEW_H

#include <algorithm>
#include <array>
#include <type_traits>
#include <vector>

#include "Buffer.h"
#include "../debug/Debug.h"

// Simple non-owning view over a 1D range of data. Useful when separating a container from the usage
// of its data.

// Data can be null. Only need assertions on things that reach into the buffer itself.

template<typename T>
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

	template<typename U>
	BufferView(Buffer<U> &buffer)
	{
		this->init(static_cast<T*>(buffer.begin()), buffer.getCount());
	}

	template<typename U>
	BufferView(const Buffer<U> &buffer)
	{
		this->init(static_cast<T*>(buffer.begin()), buffer.getCount());
	}

	template<typename U>
	BufferView(std::vector<U> &vec)
	{
		this->init(static_cast<T*>(vec.data()), static_cast<int>(vec.size()));
	}

	template<typename U>
	BufferView(const std::vector<U> &vec)
	{
		this->init(static_cast<T*>(vec.data()), static_cast<int>(vec.size()));
	}

	template<typename U, size_t Length>
	BufferView(std::array<U, Length> &arr)
	{
		this->init(static_cast<T*>(arr.data()), static_cast<int>(arr.size()));
	}

	template<typename U, size_t Length>
	BufferView(const std::array<U, Length> &arr)
	{
		this->init(static_cast<T*>(arr.data()), static_cast<int>(arr.size()));
	}

	template<typename U, size_t Length>
	BufferView(U(&arr)[Length])
	{
		this->init(static_cast<T*>(std::begin(arr)), static_cast<int>(std::size(arr)));
	}

	template<typename U, size_t Length>
	BufferView(const U(&arr)[Length])
	{
		this->init(static_cast<T*>(std::begin(arr)), static_cast<int>(std::size(arr)));
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

	template<typename U>
	void init(Buffer<U> &buffer)
	{
		this->init(static_cast<T*>(buffer.begin()), buffer.getCount());
	}

	template<typename U>
	void init(const Buffer<U> &buffer)
	{
		this->init(static_cast<T*>(buffer.begin()), buffer.getCount());
	}

	template<typename U>
	void init(std::vector<U> &vec)
	{
		this->init(static_cast<T*>(vec.data()), static_cast<int>(vec.size()));
	}

	template<typename U>
	void init(const std::vector<U> &vec)
	{
		this->init(static_cast<T*>(vec.data()), static_cast<int>(vec.size()));
	}

	template<typename U, size_t Length>
	void init(std::array<U, Length> &arr)
	{
		this->init(static_cast<T*>(arr.data()), static_cast<int>(arr.size()));
	}

	template<typename U, size_t Length>
	void init(const std::array<U, Length> &arr)
	{
		this->init(static_cast<T*>(arr.data()), static_cast<int>(arr.size()));
	}

	template<typename U, size_t Length>
	void init(U(&arr)[Length])
	{
		this->init(static_cast<T*>(std::begin(arr)), static_cast<int>(std::size(arr)));
	}

	template<typename U, size_t Length>
	void init(const U(&arr)[Length])
	{
		this->init(static_cast<T*>(std::begin(arr)), static_cast<int>(std::size(arr)));
	}

	bool isValid() const
	{
		return this->data != nullptr;
	}

	T *begin()
	{
		return this->data;
	}

	const T *begin() const
	{
		return this->data;
	}

	T *end()
	{
		return (this->data != nullptr) ? (this->data + this->count) : nullptr;
	}

	const T *end() const
	{
		return (this->data != nullptr) ? (this->data + this->count) : nullptr;
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

	T &operator[](int index)
	{
		return this->get(index);
	}

	const T &operator[](int index) const
	{
		return this->get(index);
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

		std::fill(this->begin(), this->end(), value);
	}

	void reset()
	{
		this->data = nullptr;
		this->count = 0;
	}
};

#endif
