#include "LevelInstance.h"

#include "components/debug/Debug.h"

void LevelInstance::init()
{
	this->changedVoxels.clear();
}

Int3 LevelInstance::makeVoxelCoord(WEInt x, int y, SNInt z)
{
	return Int3(x, y, z);
}

LevelInstance::ChangedVoxel *LevelInstance::findVoxel(WEInt x, int y, SNInt z)
{
	const Int3 voxelCoord = LevelInstance::makeVoxelCoord(x, y, z);
	for (ChangedVoxel &changedVoxel : this->changedVoxels)
	{
		if (changedVoxel.first == voxelCoord)
		{
			return &changedVoxel;
		}
	}

	return nullptr;
}

const LevelInstance::ChangedVoxel *LevelInstance::findVoxel(WEInt x, int y, SNInt z) const
{
	const Int3 voxelCoord = LevelInstance::makeVoxelCoord(x, y, z);
	for (const ChangedVoxel &changedVoxel : this->changedVoxels)
	{
		if (changedVoxel.first == voxelCoord)
		{
			return &changedVoxel;
		}
	}

	return nullptr;
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

LevelDefinition::VoxelID LevelInstance::getVoxel(WEInt x, int y, SNInt z,
	const LevelDefinition &levelDef) const
{
	const ChangedVoxel *changedVoxel = this->findVoxel(x, y, z);
	return (changedVoxel != nullptr) ? changedVoxel->second : levelDef.getVoxel(x, y, z);
}

void LevelInstance::setChangedVoxel(WEInt x, int y, SNInt z, LevelDefinition::VoxelID voxelID)
{
	ChangedVoxel *changedVoxel = this->findVoxel(x, y, z);

	if (changedVoxel != nullptr)
	{
		changedVoxel->second = voxelID;
	}
	else
	{
		const Int3 voxelCoord = LevelInstance::makeVoxelCoord(x, y, z);
		this->changedVoxels.push_back(std::make_pair(voxelCoord, voxelID));
	}
}
