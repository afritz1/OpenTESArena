#ifndef FLAT_MAP_H
#define FLAT_MAP_H

#include <algorithm>
#include <type_traits>
#include <vector>

// Key-value pairs with sorted insertion, fast lookup, and fast iteration. Index of a key is equal to index of its value.
template<typename KeyT, typename ValueT>
struct FlatMap
{
	static_assert(std::is_integral_v<KeyT>);
	static_assert(std::is_default_constructible_v<ValueT>);
	static_assert(std::is_move_assignable_v<ValueT>);
	static_assert(!std::is_polymorphic_v<ValueT>);

	std::vector<KeyT> keys;
	std::vector<ValueT> values;

	FlatMap() = default;

	FlatMap(int capacity)
	{
		this->keys.reserve(capacity);
		this->values.reserve(capacity);
	}

	int getCount() const
	{
		return static_cast<int>(this->keys.size());
	}

	bool isEmpty() const
	{
		return this->getCount() == 0;
	}

	std::pair<KeyT, ValueT&> getPairAtIndex(int index)
	{
		return std::pair<KeyT, ValueT&>(this->keys[index], this->values[index]);
	}

	std::pair<KeyT, const ValueT&> getPairAtIndex(int index) const
	{
		return std::pair<KeyT, const ValueT&>(this->keys[index], this->values[index]);
	}

	int findIndex(KeyT key) const
	{
		const auto iter = std::lower_bound(this->keys.begin(), this->keys.end(), key);
		if (iter == this->keys.end())
		{
			return -1;
		}

		if (*iter != key)
		{
			return -1;
		}

		return static_cast<int>(std::distance(this->keys.begin(), iter));
	}

	ValueT *find(KeyT key)
	{
		const int index = this->findIndex(key);
		if (index < 0)
		{
			return nullptr;
		}

		return &this->values[index];
	}

	const ValueT *find(KeyT key) const
	{
		const int index = this->findIndex(key);
		if (index < 0)
		{
			return nullptr;
		}

		return &this->values[index];
	}

	int emplace(KeyT key, const ValueT &value)
	{
		int index = -1;

		const auto iter = std::lower_bound(this->keys.begin(), this->keys.end(), key);
		if (iter != this->keys.end())
		{
			index = static_cast<int>(std::distance(this->keys.begin(), iter));

			const KeyT existingKey = *iter;
			if (existingKey == key)
			{
				this->values[index] = value;
			}
			else
			{
				this->keys.emplace(this->keys.begin() + index, key);
				this->values.emplace(this->values.begin() + index, value);
			}
		}
		else
		{
			index = static_cast<int>(this->keys.size());
			this->keys.emplace_back(key);
			this->values.emplace_back(value);
		}

		return index;
	}

	int emplace(KeyT key, ValueT &&value)
	{
		int index = -1;

		const auto iter = std::lower_bound(this->keys.begin(), this->keys.end(), key);
		if (iter != this->keys.end())
		{
			index = static_cast<int>(std::distance(this->keys.begin(), iter));

			const KeyT existingKey = *iter;
			if (existingKey == key)
			{
				this->values[index] = std::move(value);
			}
			else
			{
				this->keys.emplace(this->keys.begin() + index, key);
				this->values.emplace(this->values.begin() + index, std::move(value));
			}
		}
		else
		{
			index = static_cast<int>(this->keys.size());
			this->keys.emplace_back(key);
			this->values.emplace_back(std::move(value));
		}

		return index;
	}

	void erase(KeyT key)
	{
		const int index = this->findIndex(key);
		if (index < 0)
		{
			return;
		}

		this->keys.erase(this->keys.begin() + index);
		this->values.erase(this->values.begin() + index);
	}

	void clear()
	{
		this->keys.clear();
		this->values.clear();
	}
};

#endif
