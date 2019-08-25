#ifndef BUFFER_H
#define BUFFER_H

#include <algorithm>
#include <memory>

#include "../debug/Debug.h"

// Slightly cheaper alternative to vector for single-allocation uses.

template <typename T>
class Buffer
{
private:
	std::unique_ptr<T[]> data;
	int count;
public:
	Buffer()
	{
		this->count = 0;
	}

	Buffer(int count)
	{
		this->init(count);
	}

	void init(int count)
	{
		DebugAssert(count > 0);
		this->data = std::make_unique<T[]>(count);
		this->count = count;
	}

	bool isValid() const
	{
		return this->data.get() != nullptr;
	}

	T *get() const
	{
		return this->data.get();
	}

	int getCount() const
	{
		return this->count;
	}

	void set(int index, const T &value)
	{
		DebugAssert(index >= 0);
		DebugAssert(index < this->count);
		this->data.get()[index] = value;
	}

	void set(int index, T &&value)
	{
		DebugAssert(index >= 0);
		DebugAssert(index < this->count);
		this->data.get()[index] = std::move(value);
	}

	void fill(const T &value)
	{
		std::fill(this->data.get(), this->data.get() + this->count, value);
	}

	void clear()
	{
		this->data = nullptr;
		this->count = 0;
	}
};

#endif
