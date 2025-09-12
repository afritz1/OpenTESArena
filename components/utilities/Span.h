#ifndef SPAN_H
#define SPAN_H

#include <algorithm>
#include <array>
#include <vector>

#include "Buffer.h"
#include "StaticVector.h"
#include "../debug/Debug.h"

// Simple non-owning view of a 1D range of data. Useful when separating a container from the usage
// of its data. Data can be null. Only need assertions on things that reach into the buffer itself.
template<typename T>
class Span
{
private:
	T *data;
	int count;
public:
	Span()
	{
		this->reset();
	}

	// View across a subset of a range of data. Provided for bounds-checking the view range
	// inside a full range (data, data + count) at initialization.
	Span(T *data, int count, int viewOffset, int viewCount)
	{
		this->init(data, count, viewOffset, viewCount);
	}

	// View across a range of data.
	Span(T *data, int count)
	{
		this->init(data, count);
	}

	template<typename U>
	Span(Buffer<U> &buffer)
	{
		this->init(static_cast<T*>(buffer.begin()), buffer.getCount());
	}

	template<typename U>
	Span(const Buffer<U> &buffer)
	{
		this->init(static_cast<T*>(buffer.begin()), buffer.getCount());
	}

	template<typename U, int N>
	Span(StaticVector<U, N> &vec)
	{
		this->init(static_cast<T*>(vec.begin()), vec.size());
	}

	template<typename U, int N>
	Span(const StaticVector<U, N> &vec)
	{
		this->init(static_cast<T*>(vec.begin()), vec.size());
	}

	template<typename U>
	Span(std::vector<U> &vec)
	{
		this->init(static_cast<T*>(vec.data()), static_cast<int>(vec.size()));
	}

	template<typename U>
	Span(const std::vector<U> &vec)
	{
		this->init(static_cast<T*>(vec.data()), static_cast<int>(vec.size()));
	}

	template<typename U, size_t Length>
	Span(std::array<U, Length> &arr)
	{
		this->init(static_cast<T*>(arr.data()), static_cast<int>(arr.size()));
	}

	template<typename U, size_t Length>
	Span(const std::array<U, Length> &arr)
	{
		this->init(static_cast<T*>(arr.data()), static_cast<int>(arr.size()));
	}

	template<typename U, size_t Length>
	Span(U(&arr)[Length])
	{
		this->init(static_cast<T*>(std::begin(arr)), static_cast<int>(std::size(arr)));
	}

	template<typename U, size_t Length>
	Span(const U(&arr)[Length])
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

	T &operator[](int index)
	{
		DebugAssert(this->isValid());
		DebugAssert(index >= 0);
		DebugAssert(index < this->count);
		return this->data[index];
	}

	const T &operator[](int index) const
	{
		DebugAssert(this->isValid());
		DebugAssert(index >= 0);
		DebugAssert(index < this->count);
		return this->data[index];
	}

	int getCount() const
	{
		return this->count;
	}

	bool isValidRange(int startIndex, int length) const
	{
		if (!this->isValid())
		{
			return false;
		}

		if (length < 0)
		{
			return false;
		}

		const int exclusiveEndIndex = startIndex + length;
		const bool isStartValid = (startIndex >= 0) && (startIndex <= this->count);
		const bool isEndValid = (exclusiveEndIndex >= startIndex) && (exclusiveEndIndex <= this->count);
		return isStartValid && isEndValid;
	}

	Span<T> slice(int startIndex, int length)
	{
		DebugAssert(this->isValidRange(startIndex, length));
		return Span<T>(this->data + startIndex, length);
	}

	Span<const T> slice(int startIndex, int length) const
	{
		DebugAssert(this->isValidRange(startIndex, length));
		return Span<const T>(this->data + startIndex, length);
	}

	void fill(const T &value)
	{
		std::fill(this->begin(), this->end(), value);
	}

	void reset()
	{
		this->data = nullptr;
		this->count = 0;
	}
};

namespace std
{
	template<typename T>
	size_t size(const Span<T> &span)
	{
		return static_cast<size_t>(span.getCount());
	}
}

#endif
