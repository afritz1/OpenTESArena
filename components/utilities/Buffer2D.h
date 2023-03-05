#ifndef BUFFER2D_H
#define BUFFER2D_H

#include <algorithm>
#include <memory>

#include "../debug/Debug.h"

// Heap-allocated 1D array accessible as a 2D array.

// Data can be null. Only need assertions on things that reach into the buffer itself.

template <typename T>
class Buffer2D
{
private:
	std::unique_ptr<T[]> data;
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
		this->width = 0;
		this->height = 0;
	}

	Buffer2D(int width, int height)
	{
		this->init(width, height);
	}

	void init(int width, int height)
	{
		DebugAssert(width >= 0);
		DebugAssert(height >= 0);
		this->data = std::make_unique<T[]>(width * height);
		this->width = width;
		this->height = height;
	}

	bool isValid() const
	{
		return this->data != nullptr;
	}

	T *begin()
	{
		return this->data.get();
	}

	const T *begin() const
	{
		return this->data.get();
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

	T *end()
	{
		return this->isValid() ? (this->data.get() + (this->width * this->height)) : nullptr;
	}

	const T *end() const
	{
		return this->isValid() ? (this->data.get() + (this->width * this->height)) : nullptr;
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
		std::fill(this->begin(), this->end(), value);
	}

	void clear()
	{
		this->data = nullptr;
		this->width = 0;
		this->height = 0;
	}
};

#endif
