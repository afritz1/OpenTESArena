#include <algorithm>

#include "LevelInstance.h"

#include "components/debug/Debug.h"

void LevelInstance::init()
{
	this->changedVoxels.clear();
}

int LevelInstance::getChangedVoxelCount() const
{
	return static_cast<int>(this->changedVoxels.size());
}

const LevelInstance::ChangedVoxel &LevelInstance::getChangedVoxelAt(int index) const
{
	DebugAssertIndex(this->changedVoxels, index);
	return this->changedVoxels[index];
}

void LevelInstance::setChangedVoxel(const Int3 &voxel, uint16_t voxelID)
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
