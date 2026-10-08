#pragma once

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

	template<typename PredicateT>
	int findIndex(const PredicateT &predicate)
	{
		if (!this->isValid())
		{
			return -1;
		}

		for (int i = 0; i < this->count; i++)
		{
			const T &value = *(this->data + i);
			if (predicate(value))
			{
				return i;
			}
		}

		return -1;
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

// Non-owning view of a 2D range of data stored in memory as a 1D array. More sophisticated than 1D span due
// to slicing capability. Data can be null. Only need assertions on things that reach into the buffer itself.
template<typename T>
class Span2D
{
private:
	T *data; // Start of original 2D array.
	int width, height; // Dimensions of original 2D array.
	int viewX, viewY; // Start of 2D array slice.
	int viewWidth, viewHeight; // Dimensions of 2D array slice.
	bool isContiguous; // Whether all bytes are contiguous in memory, allowing for faster operations.
	bool isSliced; // Whether the view is a smaller area within the original buffer, causing it to potentially not be contiguous.

	int getIndex(int x, int y) const
	{
		DebugAssert(x >= 0);
		DebugAssert(y >= 0);
		DebugAssert(x < this->viewWidth);
		DebugAssert(y < this->viewHeight);

		if (!this->isSliced)
		{
			return x + (y * this->width);
		}
		else if (this->isContiguous)
		{
			return x + ((this->viewY + y) * this->width);
		}
		else
		{
			return (this->viewX + x) + ((this->viewY + y) * this->width);
		}
	}
public:
	Span2D()
	{
		this->reset();
	}

	// View across a subset of a 2D range of data. The original 2D range's dimensions are required
	// for proper look-up (and bounds-checking).
	Span2D(T *data, int width, int height, int viewX, int viewY, int viewWidth, int viewHeight)
	{
		this->init(data, width, height, viewX, viewY, viewWidth, viewHeight);
	}

	// View across a 2D range of data.
	Span2D(T *data, int width, int height)
	{
		this->init(data, width, height);
	}

	template<typename U>
	Span2D(Buffer2D<U> &buffer)
	{
		this->init(static_cast<T*>(buffer.begin()), buffer.getWidth(), buffer.getHeight());
	}

	template<typename U>
	Span2D(const Buffer2D<U> &buffer)
	{
		this->init(static_cast<T*>(buffer.begin()), buffer.getWidth(), buffer.getHeight());
	}

	void init(T *data, int width, int height, int viewX, int viewY, int viewWidth, int viewHeight)
	{
		DebugAssert(width >= 0);
		DebugAssert(height >= 0);
		DebugAssert(viewX >= 0);
		DebugAssert(viewY >= 0);
		DebugAssert(viewWidth >= 0);
		DebugAssert(viewHeight >= 0);
		DebugAssert((viewX + viewWidth) <= width);
		DebugAssert((viewY + viewHeight) <= height);
		this->data = data;
		this->width = width;
		this->height = height;
		this->viewX = viewX;
		this->viewY = viewY;
		this->viewWidth = viewWidth;
		this->viewHeight = viewHeight;
		this->isContiguous = viewWidth == width;
		this->isSliced = (viewWidth < width) || (viewHeight < height);
	}

	void init(T *data, int width, int height)
	{
		this->init(data, width, height, 0, 0, width, height);
	}

	template<typename U>
	void init(Buffer2D<U> &buffer)
	{
		this->init(static_cast<T*>(buffer.begin()), buffer.getWidth(), buffer.getHeight());
	}

	template<typename U>
	void init(const Buffer2D<U> &buffer)
	{
		this->init(static_cast<T*>(buffer.begin()), buffer.getWidth(), buffer.getHeight());
	}

	bool isValid() const
	{
		return this->data != nullptr;
	}

	T *begin()
	{
		DebugAssert(this->isContiguous);
		return this->data + (this->viewY * this->width);
	}

	const T *begin() const
	{
		DebugAssert(this->isContiguous);
		return this->data + (this->viewY * this->width);
	}

	T *end()
	{
		DebugAssert(this->isContiguous);
		return this->isValid() ? (this->begin() + (this->viewHeight * this->width)) : nullptr;
	}

	const T *end() const
	{
		DebugAssert(this->isContiguous);
		return this->isValid() ? (this->begin() + (this->viewHeight * this->width)) : nullptr;
	}

	T &get(int x, int y)
	{
		DebugAssert(this->isValid());
		const int index = this->getIndex(x, y);
		return this->data[index];
	}

	const T &get(int x, int y) const
	{
		DebugAssert(this->isValid());
		const int index = this->getIndex(x, y);
		return this->data[index];
	}

	int getWidth() const
	{
		return this->viewWidth;
	}

	int getHeight() const
	{
		return this->viewHeight;
	}

	void set(int x, int y, const T &value)
	{
		DebugAssert(this->isValid());
		const int index = this->getIndex(x, y);
		this->data[index] = value;
	}

	void set(int x, int y, T &&value)
	{
		DebugAssert(this->isValid());
		const int index = this->getIndex(x, y);
		this->data[index] = std::move(value);
	}

	void fill(const T &value)
	{
		if (this->isContiguous)
		{
			std::fill(this->begin(), this->end(), value);
		}
		else
		{
			for (int y = 0; y < this->viewHeight; y++)
			{
				// Elements in a row are adjacent in memory.
				T *startPtr = this->data + this->getIndex(0, y);
				T *endPtr = startPtr + this->viewWidth;
				std::fill(startPtr, endPtr, value);
			}
		}
	}

	void reset()
	{
		this->data = nullptr;
		this->width = 0;
		this->height = 0;
		this->viewX = 0;
		this->viewY = 0;
		this->viewWidth = 0;
		this->viewHeight = 0;
		this->isContiguous = false;
		this->isSliced = false;
	}
};

// Non-owning view of a 3D range of data stored in memory as a 1D array. More sophisticated than 2D span due
// to slicing capability. Data can be null. Only need assertions on things that reach into the buffer itself.
template<typename T>
class Span3D
{
private:
	T *data; // Start of original 3D array.
	int width, height, depth; // Dimensions of original 3D array.
	int viewX, viewY, viewZ; // Start of 3D array slice.
	int viewWidth, viewHeight, viewDepth; // Dimensions of 3D array slice.
	bool isContiguous; // Whether all bytes are contiguous in memory, allowing for faster operations.
	bool isSliced; // Whether the view is a smaller area within the original buffer, causing it to potentially not be contiguous.

	int getIndex(int x, int y, int z) const
	{
		DebugAssert(x >= 0);
		DebugAssert(y >= 0);
		DebugAssert(z >= 0);
		DebugAssert(x < this->viewWidth);
		DebugAssert(y < this->viewHeight);
		DebugAssert(z < this->viewDepth);

		if (!this->isSliced)
		{
			return x + (y * this->width) + (z * (this->width * this->height));
		}
		else if (this->isContiguous)
		{
			return x + (y * this->width) + ((this->viewZ + z) * (this->width * this->height));
		}
		else
		{
			return (this->viewX + x) + ((this->viewY + y) * this->width) + ((this->viewZ + z) * (this->width * this->height));
		}
	}
public:
	Span3D()
	{
		this->reset();
	}

	// View across a subset of a 3D range of data. The original 3D range's dimensions are required
	// for proper look-up (and bounds-checking).
	Span3D(T *data, int width, int height, int depth, int viewX, int viewY, int viewZ,
		int viewWidth, int viewHeight, int viewDepth)
	{
		this->init(data, width, height, depth, viewX, viewY, viewZ, viewWidth, viewHeight, viewDepth);
	}

	// View across a 3D range of data.
	Span3D(T *data, int width, int height, int depth)
	{
		this->init(data, width, height, depth);
	}

	template<typename U>
	Span3D(Buffer3D<U> &buffer)
	{
		this->init(static_cast<T*>(buffer.begin()), buffer.getWidth(), buffer.getHeight(), buffer.getDepth());
	}

	template<typename U>
	Span3D(const Buffer3D<U> &buffer)
	{
		this->init(static_cast<T*>(buffer.begin()), buffer.getWidth(), buffer.getHeight(), buffer.getDepth());
	}

	void init(T *data, int width, int height, int depth, int viewX, int viewY, int viewZ,
		int viewWidth, int viewHeight, int viewDepth)
	{
		DebugAssert(width >= 0);
		DebugAssert(height >= 0);
		DebugAssert(depth >= 0);
		DebugAssert(viewX >= 0);
		DebugAssert(viewY >= 0);
		DebugAssert(viewZ >= 0);
		DebugAssert(viewWidth >= 0);
		DebugAssert(viewHeight >= 0);
		DebugAssert(viewDepth >= 0);
		DebugAssert((viewX + viewWidth) <= width);
		DebugAssert((viewY + viewHeight) <= height);
		DebugAssert((viewZ + viewDepth) <= depth);
		this->data = data;
		this->width = width;
		this->height = height;
		this->depth = depth;
		this->viewX = viewX;
		this->viewY = viewY;
		this->viewZ = viewZ;
		this->viewWidth = viewWidth;
		this->viewHeight = viewHeight;
		this->viewDepth = viewDepth;
		this->isContiguous = (viewWidth == width) && (viewHeight == height);
		this->isSliced = (viewWidth < width) || (viewHeight < height) || (viewDepth < depth);
	}

	void init(T *data, int width, int height, int depth)
	{
		this->init(data, width, height, depth, 0, 0, 0, width, height, depth);
	}

	template<typename U>
	void init(Buffer3D<U> &buffer)
	{
		this->init(static_cast<T*>(buffer.begin()), buffer.getWidth(), buffer.getHeight(), buffer.getDepth());
	}

	template<typename U>
	void init(const Buffer3D<U> &buffer)
	{
		this->init(static_cast<T*>(buffer.begin()), buffer.getWidth(), buffer.getHeight(), buffer.getDepth());
	}

	bool isValid() const
	{
		return this->data != nullptr;
	}

	T *begin()
	{
		DebugAssert(this->isContiguous);
		return this->data + (this->viewZ * (this->width * this->height));
	}

	const T *begin() const
	{
		DebugAssert(this->isContiguous);
		return this->data + (this->viewZ * (this->width * this->height));
	}

	T *end()
	{
		DebugAssert(this->isContiguous);
		return this->isValid() ? (this->begin() + (this->viewDepth * (this->width * this->height))) : nullptr;
	}

	const T *end() const
	{
		DebugAssert(this->isContiguous);
		return this->isValid() ? (this->begin() + (this->viewDepth * (this->width * this->height))) : nullptr;
	}

	T &get(int x, int y, int z)
	{
		DebugAssert(this->isValid());
		const int index = this->getIndex(x, y, z);
		return this->data[index];
	}

	const T &get(int x, int y, int z) const
	{
		DebugAssert(this->isValid());
		const int index = this->getIndex(x, y, z);
		return this->data[index];
	}

	int getWidth() const
	{
		return this->viewWidth;
	}

	int getHeight() const
	{
		return this->viewHeight;
	}

	int getDepth() const
	{
		return this->viewDepth;
	}

	void set(int x, int y, int z, const T &value)
	{
		DebugAssert(this->isValid());
		const int index = this->getIndex(x, y, z);
		this->data[index] = value;
	}

	void set(int x, int y, int z, T &&value)
	{
		DebugAssert(this->isValid());
		const int index = this->getIndex(x, y, z);
		this->data[index] = std::move(value);
	}

	void fill(const T &value)
	{
		if (this->isContiguous)
		{
			std::fill(this->begin(), this->end(), value);
		}
		else
		{
			for (int z = 0; z < this->viewDepth; z++)
			{
				for (int y = 0; y < this->viewHeight; y++)
				{
					// Elements in a row are adjacent in memory.
					T *startPtr = this->data + this->getIndex(0, y, z);
					T *endPtr = startPtr + this->viewWidth;
					std::fill(startPtr, endPtr, value);
				}
			}
		}
	}

	void reset()
	{
		this->data = nullptr;
		this->width = 0;
		this->height = 0;
		this->depth = 0;
		this->viewX = 0;
		this->viewY = 0;
		this->viewZ = 0;
		this->viewWidth = 0;
		this->viewHeight = 0;
		this->viewDepth = 0;
		this->isContiguous = false;
		this->isSliced = false;
	}
};
