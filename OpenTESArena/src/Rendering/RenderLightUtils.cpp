#include <algorithm>
#include <limits>

#include "RenderLightUtils.h"

#include "components/debug/Debug.h"

namespace
{
	constexpr RenderLightID NO_LIGHT_ID = -1;
	constexpr double NO_DISTANCE_SQR = std::numeric_limits<double>::infinity();
}

RenderLightIdList::RenderLightIdList()
{
	this->clear();
}

BufferView<const RenderLightID> RenderLightIdList::getLightIDs() const
{
	return BufferView<const RenderLightID>(this->lightIDs, this->lightCount);
}

void RenderLightIdList::tryAddLight(RenderLightID id, double distanceSqr)
{
	DebugAssert(distanceSqr >= 0.0);

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

	// Find insertion index so IDs remain sorted.
	int insertIndex = 0;
	for (int i = 0; i < this->lightCount; i++)
	{
		const double curDistanceSqr = this->distanceSqrs[i];
		if (distanceSqr < curDistanceSqr)
		{
			insertIndex = i;
			break;
		}
	}

	// Shift elements back.
	for (int i = this->lightCount; i > insertIndex; i--)
	{
		this->lightIDs[i] = this->lightIDs[i - 1];
		this->distanceSqrs[i] = this->distanceSqrs[i - 1];
	}

	this->lightIDs[insertIndex] = id;
	this->distanceSqrs[insertIndex] = distanceSqr;
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
		this->distanceSqrs[i - 1] = this->distanceSqrs[i];
	}

	this->lightIDs[this->lightCount - 1] = NO_LIGHT_ID;
	this->distanceSqrs[this->lightCount - 1] = NO_DISTANCE_SQR;
	this->lightCount--;
}

void RenderLightIdList::removeLight(RenderLightID id)
{
	DebugAssert(id != NO_LIGHT_ID);

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
	std::fill(std::begin(this->lightIDs), std::end(this->lightIDs), NO_LIGHT_ID);
	std::fill(std::begin(this->distanceSqrs), std::end(this->distanceSqrs), NO_DISTANCE_SQR);
	this->lightCount = 0;
}
