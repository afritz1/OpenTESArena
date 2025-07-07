#ifndef STATIC_VECTOR_H
#define STATIC_VECTOR_H

#include <initializer_list>
#include <type_traits>

#include "../debug/Debug.h"

// Stack-based vector with inserting and erasing. Elements are default-constructed.
// (tried a std::byte[] version with placement new but was incomprehensible in the debugger)
template<typename T, int N>
class StaticVector
{
private:
	T values[N];
	int count;
public:
	constexpr StaticVector()
	{
		this->count = 0;
	}

	constexpr StaticVector(std::initializer_list<T> list)
		: StaticVector()
	{
		for (const T &value : list)
		{
			this->emplaceBack(value);
		}
	}

	constexpr T *begin()
	{
		return this->values;
	}

	constexpr const T *begin() const
	{
		return this->values;
	}

	constexpr T *end()
	{
		return this->values + this->count;
	}

	constexpr const T *end() const
	{
		return this->values + this->count;
	}

	constexpr int size() const
	{
		return this->count;
	}

	constexpr int capacity() const
	{
		return N;
	}

	constexpr bool empty() const
	{
		return this->count == 0;
	}

	constexpr T &operator[](int index)
	{
		DebugAssert(index >= 0);
		DebugAssert(index < this->count);
		return this->values[index];
	}

	constexpr const T &operator[](int index) const
	{
		DebugAssert(index >= 0);
		DebugAssert(index < this->count);
		return this->values[index];
	}

	constexpr T &back()
	{
		DebugAssert(this->count > 0);
		return this->values[this->count - 1];
	}

	constexpr const T &back() const
	{
		DebugAssert(this->count > 0);
		return this->values[this->count - 1];
	}

	constexpr void emplaceBack(const T &value)
	{
		DebugAssert(this->count < N);
		this->values[this->count] = value;
		this->count++;
	}

	constexpr void emplaceBack(T &&value)
	{
		DebugAssert(this->count < N);
		this->values[this->count] = std::move(value);
		this->count++;
	}

	template<typename... Args>
	constexpr void emplaceBack(Args&&... args)
	{
		DebugAssert(this->count < N);
		this->values[this->count] = T(std::forward<Args>(args)...);
		this->count++;
	}

	constexpr void insert(int index, const T &value)
	{
		DebugAssert(this->count < N);
		DebugAssert(index >= 0);
		DebugAssert(index <= this->count);

		for (int i = this->count; i > index; i--)
		{
			this->values[i] = std::move(this->values[i - 1]);
		}

		this->values[index] = value;
		this->count++;
	}

	constexpr void insert(int index, T &&value)
	{
		DebugAssert(this->count < N);
		DebugAssert(index >= 0);
		DebugAssert(index <= this->count);

		for (int i = this->count; i > index; i--)
		{
			this->values[i] = std::move(this->values[i - 1]);
		}

		this->values[index] = std::move(value);
		this->count++;
	}

	constexpr void erase(int index)
	{
		DebugAssert(index >= 0);
		DebugAssert(index < this->count);

		for (int i = index + 1; i < this->count; i++)
		{
			this->values[i - 1] = std::move(this->values[i]);
		}

		this->count--;
	}

	constexpr void popBack()
	{
		DebugAssert(this->count > 0);
		this->count--;
	}

	constexpr void clear()
	{
		this->count = 0;
	}
};

#endif
