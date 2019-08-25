#ifndef BUFFER3D_H
#define BUFFER3D_H

#include <memory>

#include "../debug/Debug.h"

// Heap-allocated 1D array accessible as a 3D array.

template <typename T>
class Buffer3D
{
private:
	std::unique_ptr<T[]> data;
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
		this->width = 0;
		this->height = 0;
		this->depth = 0;
	}

	Buffer3D(int width, int height, int depth)
	{
		this->init(width, height, depth);
	}

	void init(int width, int height, int depth)
	{
		DebugAssert(width > 0);
		DebugAssert(height > 0);
		DebugAssert(depth > 0);
		this->data = std::make_unique<T[]>(width * height * depth);
		this->width = width;
		this->height = height;
		this->depth = depth;
	}

	bool isValid() const
	{
		return this->data.get() != nullptr;
	}

	T *get() const
	{
		return this->data.get();
	}

	T *get(int x, int y, int z) const
	{
		const int index = this->getIndex(x, y, z);
		return this->data.get() + index;
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
		const int index = this->getIndex(x, y, z);
		this->data.get()[index] = value;
	}

	void set(int x, int y, int z, T &&value)
	{
		const int index = this->getIndex(x, y, z);
		this->data.get()[index] = std::move(value);
	}

	void clear()
	{
		this->data = nullptr;
		this->width = 0;
		this->height = 0;
		this->depth = 0;
	}
};

#endif
