#include "LevelInstance.h"

#include "components/debug/Debug.h"

void LevelInstance::init()
{
	this->changedVoxels.clear();
	this->voxelInsts.clear();
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

const LevelInstance::ChangedVoxel &LevelInstance::getChangedVoxel(int index) const
{
	DebugAssertIndex(this->changedVoxels, index);
	return this->changedVoxels[index];
}

const VoxelDefinition &LevelInstance::getVoxelDef(LevelDefinition::VoxelID id,
	const LevelDefinition &levelDef) const
{
	const int baseVoxelDefCount = levelDef.getVoxelDefCount();
	const bool useBaseVoxelDefs = id < baseVoxelDefCount;
	if (useBaseVoxelDefs)
	{
		return levelDef.getVoxelDef(id);
	}
	else
	{
		const int index = static_cast<int>(id) - baseVoxelDefCount;
		DebugAssertIndex(this->voxelDefAdditions, index);
		return this->voxelDefAdditions[index];
	}
}

int LevelInstance::getVoxelInstanceCount() const
{
	return static_cast<int>(this->voxelInsts.size());
}

VoxelInstance &LevelInstance::getVoxelInstance(int index)
{
	DebugAssertIndex(this->voxelInsts, index);
	return this->voxelInsts[index];
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

LevelDefinition::VoxelID LevelInstance::addVoxelDef(const VoxelDefinition &voxelDef)
{
	this->voxelDefAdditions.push_back(voxelDef);
	return static_cast<LevelDefinition::VoxelID>(this->voxelDefAdditions.size() - 1);
}

void LevelInstance::update(double dt)
{
	// @todo: reverse iterate over voxel instances, removing ones that are finished doing
	// their animation, etc.. See LevelData::updateFadingVoxels() for reference.
	// @todo: when updating adjacent chasms, add new voxel definitions to the level definition
	// if necessary. It's okay to mutate the level def because new chasm permutations aren't
	// really "instance data" -- they belong in the level def in the first place.
	/*for (VoxelInstance &voxelInst : this->voxelInsts)
	{
		voxelInst.update(dt);
	}*/

	DebugNotImplemented();
}
