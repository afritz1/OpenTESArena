#ifndef FIXED_POOL_H
#define FIXED_POOL_H

#include <algorithm>

#include "../debug/Debug.h"

// Allocates up to a fixed number of indices into values, each slot can be freed and reused.
template<typename T, int Count>
struct FixedPool
{
	T values[Count]; // Might contain freed slots.
	int nextValueIndex;

	int freedIndices[Count];
	int freedIndexCount;

	FixedPool()
	{
		this->clear();
	}

	int getUsedCount() const
	{
		return this->nextValueIndex - this->freedIndexCount;
	}

	int alloc()
	{
		int valueIndex = -1;

		if (this->freedIndexCount > 0)
		{
			this->freedIndexCount--;
			valueIndex = this->freedIndices[this->freedIndexCount];
			this->freedIndices[this->freedIndexCount] = -1;
		}
		else
		{
			if (this->nextValueIndex < Count)
			{
				valueIndex = this->nextValueIndex;
				this->nextValueIndex++;
			}
			else
			{
				DebugLogError("No more free indices to allocate.");
			}
		}

		return valueIndex;
	}

	void free(int index)
	{
		DebugAssert(index >= 0);
		DebugAssert(index < Count);

		if ((index < 0) || (index >= this->nextValueIndex))
		{
			DebugLogErrorFormat("Invalid index %d to free.", index);
			return;
		}

		if (this->freedIndexCount == Count)
		{
			DebugLogErrorFormat("Can't free index %d because the max number of indices have been freed.", index);
			return;
		}

		this->freedIndices[this->freedIndexCount] = index;
		this->freedIndexCount++;
	}

	void clear()
	{
		this->nextValueIndex = 0;
		std::fill(std::begin(this->freedIndices), std::end(this->freedIndices), -1);
		this->freedIndexCount = 0;
	}
};

#endif
