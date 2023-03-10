#ifndef BUFFER_H
#define BUFFER_H

#include <algorithm>
#include <memory>

#include "../debug/Debug.h"

// Slightly cheaper alternative to vector for single-allocation uses.

// Data can be null. Only need assertions on things that reach into the buffer itself.

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
		DebugAssert(count >= 0);
		this->data = std::make_unique<T[]>(count);
		this->count = count;
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

	T *end()
	{
		return this->isValid() ? (this->data.get() + this->count) : nullptr;
	}

	const T *end() const
	{
		return this->isValid() ? (this->data.get() + this->count) : nullptr;
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
		DebugAssert(this->isValid());
		DebugAssert(index >= 0);
		DebugAssert(index < this->count);
		this->data[index] = value;
	}

	void set(int index, T &&value)
	{
		DebugAssert(this->isValid());
		DebugAssert(index >= 0);
		DebugAssert(index < this->count);
		this->data[index] = std::move(value);
	}

	void fill(const T &value)
	{
		std::fill(this->begin(), this->end(), value);
	}

	void clear()
	{
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

#endif
