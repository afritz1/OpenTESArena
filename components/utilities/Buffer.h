#ifndef BUFFER_H
#define BUFFER_H

#include <algorithm>
#include <initializer_list>

#include "../debug/Debug.h"

// Slightly cheaper alternative to vector for single-allocation uses.
// Data can be null. Only need assertions on things that reach into the buffer itself.

template<typename T>
class Buffer
{
private:
	T *data;
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

#endif
