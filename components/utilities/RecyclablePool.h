#ifndef RECYCLABLE_POOL_H
#define RECYCLABLE_POOL_H

#include <algorithm>
#include <type_traits>
#include <unordered_set>
#include <vector>

#include "../debug/Debug.h"

// Contiguous pool that allows elements to be freed and their position reused by future elements
// without affecting other elements.

template <typename ElementT, typename IdT>
class RecyclablePool
{
private:
	static_assert(std::is_default_constructible_v<ElementT>);
	static_assert(std::is_move_assignable_v<ElementT>);
	static_assert(std::is_standard_layout_v<ElementT>);
	static_assert(std::is_integral_v<IdT>);
	
	std::vector<ElementT> elements;
	std::unordered_set<IdT> freedIDs;
	IdT nextID;

	bool isFreedID(IdT id) const
	{
		return this->freedIDs.find(id) != this->freedIDs.end();
	}

	bool isValidID(IdT id) const
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

	ElementT &get(IdT id)
	{
		DebugAssert(this->isValidID(id));
		DebugAssertIndex(this->elements, id);
		return this->elements[id];
	}

	const ElementT &get(IdT id) const
	{
		DebugAssert(this->isValidID(id));
		DebugAssertIndex(this->elements, id);
		return this->elements[id];
	}

	bool tryAlloc(IdT *outID)
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
			this->elements.emplace_back(ElementT());
		}

		return true;
	}

	void free(IdT id)
	{
		if (!this->isValidID(id))
		{
			DebugCrash("Invalid ID to free: \"" + std::to_string(id) + "\"");
		}

		this->freedIDs.emplace(id);
		
		DebugAssertIndex(this->elements, id);
		this->elements[id] = ElementT();
	}

	void clear()
	{
		this->elements.clear();
		this->freedIDs.clear();
		this->nextID = 0;
	}
};

#endif
