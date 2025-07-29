#ifndef RECYCLABLE_POOL_H
#define RECYCLABLE_POOL_H

#include <algorithm>
#include <type_traits>
#include <vector>

#include "../debug/Debug.h"

// Contiguous pool that allows elements to be freed and their position reused by future elements
// without affecting other elements.
template<typename KeyT, typename ValueT>
struct RecyclablePool
{
	static_assert(std::is_integral_v<KeyT>);
	static_assert(std::is_default_constructible_v<ValueT>);
	static_assert(std::is_move_assignable_v<ValueT>);
	static_assert(!std::is_polymorphic_v<ValueT>);

	std::vector<KeyT> keys; // Dense list, all slots are always valid. Equal length with values list.
	std::vector<ValueT> values; // Dense list, all slots are always valid.
	std::vector<int> valueIndices; // Maps KeyT to index into elements or -1 if freed.
	std::vector<KeyT> freedKeys;

	int getCount() const
	{
		return static_cast<int>(this->values.size());
	}

	bool isFreedKey(KeyT key) const
	{
		return this->valueIndices[key] < 0;
	}

	bool isValidKey(KeyT key) const
	{
		if ((key < 0) || (key >= static_cast<int>(this->valueIndices.size())))
		{
			return false;
		}

		if (this->isFreedKey(key))
		{
			return false;
		}

		return true;
	}

	ValueT &get(KeyT key)
	{
		DebugAssertIndex(this->valueIndices, key);
		const int valueIndex = this->valueIndices[key];

		DebugAssertIndex(this->values, valueIndex);
		return this->values[valueIndex];
	}

	const ValueT &get(KeyT key) const
	{
		DebugAssertIndex(this->valueIndices, key);
		const int valueIndex = this->valueIndices[key];

		DebugAssertIndex(this->values, valueIndex);
		return this->values[valueIndex];
	}

	ValueT *tryGet(KeyT key)
	{
		if (!this->isValidKey(key))
		{
			return nullptr;
		}

		const int valueIndex = this->valueIndices[key];
		DebugAssertIndex(this->values, valueIndex);
		return &this->values[valueIndex];
	}

	const ValueT *tryGet(KeyT key) const
	{
		if (!this->isValidKey(key))
		{
			return nullptr;
		}

		const int valueIndex = this->valueIndices[key];
		DebugAssertIndex(this->values, valueIndex);
		return &this->values[valueIndex];
	}

	// Allocates a new value and returns its unique key or -1 on failure.
	KeyT alloc()
	{
		KeyT key;

		if (!this->freedKeys.empty())
		{
			key = this->freedKeys.back();
			this->freedKeys.pop_back();
			DebugAssert(this->valueIndices[key] == -1);
			this->valueIndices[key] = static_cast<int>(this->values.size());
		}
		else
		{
			key = static_cast<KeyT>(this->values.size());
			this->valueIndices.emplace_back(static_cast<int>(key));
		}

		this->keys.emplace_back(key);
		this->values.emplace_back(ValueT());

		return key;
	}

	void free(KeyT key)
	{
		if (!this->isValidKey(key))
		{
			DebugLogErrorFormat("Invalid key %d to free.", key);
			return;
		}

		const int index = this->valueIndices[key];
		const int lastIndex = static_cast<int>(this->values.size()) - 1;
		if (index != lastIndex)
		{
			const KeyT lastIndexKey = this->keys[lastIndex];
			this->valueIndices[lastIndexKey] = index;

			DebugAssertIndex(this->values, index);
			std::swap(this->keys[index], this->keys[lastIndex]);
			std::swap(this->values[index], this->values[lastIndex]);
		}

		this->keys.pop_back();
		this->values.pop_back();
		this->valueIndices[key] = -1;
		this->freedKeys.emplace_back(key);
	}

	void clear()
	{
		this->keys.clear();
		this->values.clear();
		this->valueIndices.clear();
		this->freedKeys.clear();
	}
};

#endif
