#include <algorithm>

#include "RenderLightUtils.h"

#include "components/debug/Debug.h"

RenderLightIdList::RenderLightIdList()
{
	this->clear();
}

BufferView<const RenderLightID> RenderLightIdList::getLightIDs() const
{
	return BufferView<const RenderLightID>(this->lightIDs, this->lightCount);
}

void RenderLightIdList::tryAddLight(RenderLightID id)
{
	if (this->lightCount >= static_cast<int>(std::size(this->lightIDs)))
	{
		return;
	}

	const auto beginIter = std::begin(this->lightIDs);
	const auto endIter = beginIter + this->lightCount;
	const auto existsIter = std::find(beginIter, endIter, id);
	if (existsIter != endIter)
	{
		DebugLogWarning("Light ID " + std::to_string(id) + " already in list.");
		return;
	}

	this->lightIDs[this->lightCount] = id;
	this->lightCount++;
}

void RenderLightIdList::removeLightAt(int index)
{
	DebugAssert(index >= 0);
	DebugAssert(index < this->lightCount);
	DebugAssert(this->lightCount > 0);

	for (int i = index + 1; i < this->lightCount; i++)
	{
		this->lightIDs[i - 1] = this->lightIDs[i];
	}

	this->lightIDs[this->lightCount - 1] = -1;
	this->lightCount--;
}

void RenderLightIdList::removeLight(RenderLightID id)
{
	int index = -1;
	for (int i = 0; i < this->lightCount; i++)
	{
		if (this->lightIDs[i] == id)
		{
			index = i;
			break;
		}
	}

	if (index >= 0)
	{
		this->removeLightAt(index);
	}
}

void RenderLightIdList::clear()
{
	std::fill(std::begin(this->lightIDs), std::end(this->lightIDs), -1);
	this->lightCount = 0;
}
