#pragma once

#include <algorithm>
#include <initializer_list>

#include "../debug/Debug.h"

// Slightly cheaper alternative to vector for single-allocation uses.
template<typename T>
class Buffer
{
private:
	T *data; // Can be null.
	int count;
public:
	Buffer()
	{
		this->data = nullptr;
		this->count = 0;
	}

	Buffer(int count)
	{
		DebugAssert(count >= 0);
		this->data = new T[count];
		this->count = count;
	}

	Buffer(std::initializer_list<T> list)
	{
		const int count = static_cast<int>(list.size());
		this->data = new T[count];
		this->count = count;
		std::copy(list.begin(), list.end(), this->begin());
	}

	Buffer(Buffer<T> &&other)
	{
		this->data = other.data;
		this->count = other.count;
		other.data = nullptr;
		other.count = 0;
	}

	Buffer &operator=(Buffer<T> &&other)
	{
		if (this == &other)
		{
			return *this;
		}

		if (this->isValid())
		{
			this->clear();
		}

		this->data = other.data;
		this->count = other.count;
		other.data = nullptr;
		other.count = 0;
		return *this;
	}

	Buffer(const Buffer<T>&) = delete;
	Buffer &operator=(const Buffer<T>&) = delete;

	~Buffer()
	{
		this->clear();
	}

	void init(int count)
	{
		DebugAssert(count >= 0);

		if (this->isValid())
		{
			this->clear();
		}

		this->data = new T[count];
		this->count = count;
	}

	void init(std::initializer_list<T> list)
	{
		this->init(static_cast<int>(list.size()));
		std::copy(list.begin(), list.end(), this->begin());
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
		return this->isValid() ? (this->data + this->count) : nullptr;
	}

	const T *end() const
	{
		return this->isValid() ? (this->data + this->count) : nullptr;
	}

	T &get(int index)
	{
		DebugAssert(this->isValid());
		DebugAssert(index >= 0);
		DebugAssert(index < this->count);
		return *(this->data + index);
	}

	const T &get(int index) const
	{
		DebugAssert(this->isValid());
		DebugAssert(index >= 0);
		DebugAssert(index < this->count);
		return *(this->data + index);
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
		DebugAssert(this->isValid());
		DebugAssert(index >= 0);
		DebugAssert(index < this->count);
		*(this->data + index) = value;
	}

	void set(int index, T &&value)
	{
		DebugAssert(this->isValid());
		DebugAssert(index >= 0);
		DebugAssert(index < this->count);
		*(this->data + index) = std::move(value);
	}

	void fill(const T &value)
	{
		std::fill(this->begin(), this->end(), value);
	}

	void clear()
	{
		delete[] this->data;
		this->data = nullptr;
		this->count = 0;
	}
};

namespace std
{
	template<typename T>
	size_t size(const Buffer<T> &buffer)
	{
		return static_cast<size_t>(buffer.getCount());
	}
}

// Heap-allocated 1D array accessible as a 2D array.
template<typename T>
class Buffer2D
{
private:
	T *data; // Can be null.
	int width, height;

	int getIndex(int x, int y) const
	{
		DebugAssert(x >= 0);
		DebugAssert(y >= 0);
		DebugAssert(x < this->width);
		DebugAssert(y < this->height);
		return x + (y * this->width);
	}
public:
	Buffer2D()
	{
		this->data = nullptr;
		this->width = 0;
		this->height = 0;
	}

	Buffer2D(int width, int height)
	{
		DebugAssert(width >= 0);
		DebugAssert(height >= 0);
		this->data = new T[width * height];
		this->width = width;
		this->height = height;
	}

	Buffer2D(Buffer2D<T> &&other)
	{
		this->data = other.data;
		this->width = other.width;
		this->height = other.height;
		other.data = nullptr;
		other.width = 0;
		other.height = 0;
	}

	Buffer2D &operator=(Buffer2D<T> &&other)
	{
		if (this == &other)
		{
			return *this;
		}

		if (this->isValid())
		{
			this->clear();
		}

		this->data = other.data;
		this->width = other.width;
		this->height = other.height;
		other.data = nullptr;
		other.width = 0;
		other.height = 0;
		return *this;
	}

	Buffer2D(const Buffer2D<T>&) = delete;
	Buffer2D &operator=(const Buffer2D<T>&) = delete;

	~Buffer2D()
	{
		this->clear();
	}

	void init(int width, int height)
	{
		DebugAssert(width >= 0);
		DebugAssert(height >= 0);

		if (this->isValid())
		{
			this->clear();
		}

		this->data = new T[width * height];
		this->width = width;
		this->height = height;
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
		return this->isValid() ? (this->data + (this->width * this->height)) : nullptr;
	}

	const T *end() const
	{
		return this->isValid() ? (this->data + (this->width * this->height)) : nullptr;
	}

	T &get(int x, int y)
	{
		DebugAssert(this->isValid());
		const int index = this->getIndex(x, y);
		return *(this->data + index);
	}

	const T &get(int x, int y) const
	{
		DebugAssert(this->isValid());
		const int index = this->getIndex(x, y);
		return *(this->data + index);
	}

	int getWidth() const
	{
		return this->width;
	}

	int getHeight() const
	{
		return this->height;
	}

	void set(int x, int y, const T &value)
	{
		DebugAssert(this->isValid());
		const int index = this->getIndex(x, y);
		*(this->data + index) = value;
	}

	void set(int x, int y, T &&value)
	{
		DebugAssert(this->isValid());
		const int index = this->getIndex(x, y);
		*(this->data + index) = std::move(value);
	}

	void fill(const T &value)
	{
		std::fill(this->begin(), this->end(), value);
	}

	void clear()
	{
		delete[] this->data;
		this->data = nullptr;
		this->width = 0;
		this->height = 0;
	}
};

// Heap-allocated 1D array accessible as a 3D array.
template<typename T>
class Buffer3D
{
private:
	T *data; // Can be null.
	int width, height, depth;

	int getIndex(int x, int y, int z) const
	{
		DebugAssert(x >= 0);
		DebugAssert(y >= 0);
		DebugAssert(z >= 0);
		DebugAssert(x < this->width);
		DebugAssert(y < this->height);
		DebugAssert(z < this->depth);
		return x + (y * this->width) + (z * this->width * this->height);
	}
public:
	Buffer3D()
	{
		this->data = nullptr;
		this->width = 0;
		this->height = 0;
		this->depth = 0;
	}

	Buffer3D(int width, int height, int depth)
	{
		DebugAssert(width >= 0);
		DebugAssert(height >= 0);
		DebugAssert(depth >= 0);
		this->data = new T[width * height * depth];
		this->width = width;
		this->height = height;
		this->depth = depth;
	}

	Buffer3D(Buffer3D<T> &&other)
	{
		this->data = other.data;
		this->width = other.width;
		this->height = other.height;
		this->depth = other.depth;
		other.data = nullptr;
		other.width = 0;
		other.height = 0;
		other.depth = 0;
	}

	Buffer3D &operator=(Buffer3D<T> &&other)
	{
		if (this == &other)
		{
			return *this;
		}

		if (this->isValid())
		{
			this->clear();
		}

		this->data = other.data;
		this->width = other.width;
		this->height = other.height;
		this->depth = other.depth;
		other.data = nullptr;
		other.width = 0;
		other.height = 0;
		other.depth = 0;
		return *this;
	}

	Buffer3D(const Buffer3D<T>&) = delete;
	Buffer3D &operator=(const Buffer3D<T>&) = delete;

	~Buffer3D()
	{
		this->clear();
	}

	void init(int width, int height, int depth)
	{
		DebugAssert(width >= 0);
		DebugAssert(height >= 0);
		DebugAssert(depth >= 0);

		if (this->isValid())
		{
			this->clear();
		}

		this->data = new T[width * height * depth];
		this->width = width;
		this->height = height;
		this->depth = depth;
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
		return this->isValid() ? (this->data + (this->width * this->height * this->depth)) : nullptr;
	}

	const T *end() const
	{
		return this->isValid() ? (this->data + (this->width * this->height * this->depth)) : nullptr;
	}

	T &get(int x, int y, int z)
	{
		DebugAssert(this->isValid());
		const int index = this->getIndex(x, y, z);
		return *(this->data + index);
	}

	const T &get(int x, int y, int z) const
	{
		DebugAssert(this->isValid());
		const int index = this->getIndex(x, y, z);
		return *(this->data + index);
	}

	int getWidth() const
	{
		return this->width;
	}

	int getHeight() const
	{
		return this->height;
	}

	int getDepth() const
	{
		return this->depth;
	}

	void set(int x, int y, int z, const T &value)
	{
		DebugAssert(this->isValid());
		const int index = this->getIndex(x, y, z);
		*(this->data + index) = value;
	}

	void set(int x, int y, int z, T &&value)
	{
		DebugAssert(this->isValid());
		const int index = this->getIndex(x, y, z);
		*(this->data + index) = std::move(value);
	}

	void fill(const T &value)
	{
		std::fill(this->begin(), this->end(), value);
	}

	void clear()
	{
		delete[] this->data;
		this->data = nullptr;
		this->width = 0;
		this->height = 0;
		this->depth = 0;
	}
};
