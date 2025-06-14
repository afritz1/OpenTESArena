#ifndef RECYCLABLE_POOL_H
#define RECYCLABLE_POOL_H

#include <algorithm>
#include <type_traits>
#include <unordered_set>
#include <vector>

#include "../debug/Debug.h"

// Contiguous pool that allows elements to be freed and their position reused by future elements
// without affecting other elements.
template<typename KeyT, typename ValueT>
class RecyclablePool
{
private:
	static_assert(std::is_integral_v<KeyT>);
	static_assert(std::is_default_constructible_v<ValueT>);
	static_assert(std::is_move_assignable_v<ValueT>);
	static_assert(!std::is_polymorphic_v<ValueT>);
	
	std::vector<ValueT> elements;
	std::unordered_set<KeyT> freedIDs;
	KeyT nextID;

	bool isFreedID(KeyT id) const
	{
		return this->freedIDs.find(id) != this->freedIDs.end();
	}

	bool isValidID(KeyT id) const
	{
		if ((id < 0) || (id >= this->nextID))
		{
			return false;
		}

		if (this->isFreedID(id))
		{
			return false;
		}

		return true;
	}
public:
	RecyclablePool()
	{
		this->nextID = 0;
	}

	// Gets total number of slots; not all are always in use.
	int getTotalCount() const
	{
		return static_cast<int>(this->elements.size());
	}

	int getFreeCount() const
	{
		return static_cast<int>(this->freedIDs.size());
	}

	int getUsedCount() const
	{
		return this->getTotalCount() - this->getFreeCount();
	}

	ValueT &get(KeyT id)
	{
		DebugAssert(this->isValidID(id));
		DebugAssertIndex(this->elements, id);
		return this->elements[id];
	}

	const ValueT &get(KeyT id) const
	{
		DebugAssert(this->isValidID(id));
		DebugAssertIndex(this->elements, id);
		return this->elements[id];
	}

	ValueT *tryGet(KeyT id)
	{
		if (!this->isValidID(id))
		{
			return nullptr;
		}

		DebugAssertIndex(this->elements, id);
		return &this->elements[id];
	}

	const ValueT *tryGet(KeyT id) const
	{
		if (!this->isValidID(id))
		{
			return nullptr;
		}

		DebugAssertIndex(this->elements, id);
		return &this->elements[id];
	}

	bool tryAlloc(KeyT *outID)
	{
		if (!this->freedIDs.empty())
		{
			*outID = *this->freedIDs.begin();
			this->freedIDs.erase(this->freedIDs.begin());
		}
		else
		{
			*outID = this->nextID;
			this->nextID++;
			this->elements.emplace_back(ValueT());
		}

		return true;
	}

	void free(KeyT id)
	{
		if (!this->isValidID(id))
		{
			DebugCrash("Invalid ID to free: \"" + std::to_string(id) + "\"");
		}

		this->freedIDs.emplace(id);
		
		DebugAssertIndex(this->elements, id);
		this->elements[id] = ValueT();
	}

	void clear()
	{
		this->elements.clear();
		this->freedIDs.clear();
		this->nextID = 0;
	}
};

#endif
