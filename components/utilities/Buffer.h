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
		DebugAssert(count >= 0);
		this->data = std::make_unique<T[]>(count);
		this->count = count;
	}

	bool isValid() const
	{
		return this->data.get() != nullptr;
	}

	T *get()
	{
		DebugAssert(this->isValid());
		return this->data.get();
	}

	const T *get() const
	{
		DebugAssert(this->isValid());
		return this->data.get();
	}

	T &get(int index)
	{
		DebugAssert(this->isValid());
		DebugAssert(index >= 0);
		DebugAssert(index < this->count);
		return this->data.get()[index];
	}

	const T &get(int index) const
	{
		DebugAssert(this->isValid());
		DebugAssert(index >= 0);
		DebugAssert(index < this->count);
		return this->data.get()[index];
	}

	T *end()
	{
		DebugAssert(this->isValid());
		return this->data.get() + this->count;
	}

	const T *end() const
	{
		DebugAssert(this->isValid());
		return this->data.get() + this->count;
	}

	int getCount() const
	{
		DebugAssert(this->isValid());
		return this->count;
	}

	void set(int index, const T &value)
	{
		DebugAssert(this->isValid());
		DebugAssert(index >= 0);
		DebugAssert(index < this->count);
		this->data.get()[index] = value;
	}

	void set(int index, T &&value)
	{
		DebugAssert(this->isValid());
		DebugAssert(index >= 0);
		DebugAssert(index < this->count);
		this->data.get()[index] = std::move(value);
	}

	void fill(const T &value)
	{
		DebugAssert(this->isValid());
		std::fill(this->data.get(), this->end(), value);
	}

	void clear()
	{
		this->data = nullptr;
		this->count = 0;
	}
};

#endif
