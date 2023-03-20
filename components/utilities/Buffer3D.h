#ifndef BUFFER3D_H
#define BUFFER3D_H

#include <algorithm>

#include "../debug/Debug.h"

// Heap-allocated 1D array accessible as a 3D array.
// Data can be null. Only need assertions on things that reach into the buffer itself.

template<typename T>
class Buffer3D
{
private:
	T *data;
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
		this->init(width, height, depth);
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

#endif
