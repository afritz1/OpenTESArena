#include <algorithm>

#include "RenderLightUtils.h"

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

	this->lightIDs[this->lightCount] = id;
	this->lightCount++;
}

void RenderLightIdList::clear()
{
	std::fill(std::begin(this->lightIDs), std::end(this->lightIDs), -1);
	this->lightCount = 0;
}
