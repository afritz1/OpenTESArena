#ifndef BUFFER_H
#define BUFFER_H

#include <memory>

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
		this->data = std::make_unique<T[]>(count);
		this->count = count;
	}

	T *get() const
	{
		return this->data.get();
	}

	int getCount() const
	{
		return this->count;
	}

	void clear()
	{
		this->data = nullptr;
		this->count = 0;
	}
};

#endif
