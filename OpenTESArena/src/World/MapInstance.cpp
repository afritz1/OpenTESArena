#include <algorithm>

#include "MapInstance.h"

#include "components/debug/Debug.h"

void MapInstance::Level::init()
{
	this->changedVoxels.clear();
}

int MapInstance::Level::getChangedVoxelCount() const
{
	return static_cast<int>(this->changedVoxels.size());
}

const MapInstance::Level::ChangedVoxel &MapInstance::Level::getChangedVoxelAt(int index) const
{
	DebugAssertIndex(this->changedVoxels, index);
	return this->changedVoxels[index];
}

void MapInstance::Level::setChangedVoxel(const Int3 &voxel, uint16_t voxelID)
{
	const auto iter = std::find_if(this->changedVoxels.begin(), this->changedVoxels.end(),
		[&voxel](const ChangedVoxel &changedVoxel)
	{
		return changedVoxel.first == voxel;
	});

	if (iter != this->changedVoxels.end())
	{
		iter->second = voxelID;
	}
	else
	{
		this->changedVoxels.push_back(std::make_pair(voxel, voxelID));
	}
}

void MapInstance::init(int levelCount)
{
	this->levels.init(levelCount);
}

MapInstance::Level &MapInstance::getLevel(int index)
{
	return this->levels.get(index);
}
