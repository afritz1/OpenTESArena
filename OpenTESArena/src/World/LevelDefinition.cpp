#include <algorithm>

#include "LevelDefinition.h"
#include "LevelUtils.h"

void LevelDefinition::initInterior(const MIFFile::Level &level, WEInt mifWidth, SNInt mifDepth,
	const INFFile::CeilingData *ceiling)
{
	// Determine level height from voxel data.
	const int height = [&level, mifWidth, mifDepth, ceiling]()
	{
		const BufferView2D<const MIFFile::VoxelID> map2 = level.getMAP2();

		if (map2.isValid())
		{
			return 2 + LevelUtils::getMap2Height(map2);
		}
		else
		{
			const bool hasCeiling = (ceiling != nullptr) && !ceiling->outdoorDungeon;
			return hasCeiling ? 3 : 2;
		}
	}();

	this->voxels.init(mifDepth, height, mifWidth);
	this->voxels.fill(LevelDefinition::VOXEL_ID_AIR);

	// @todo: decode level voxels and put IDs/entities into buffers.
	// - leaning towards having a helper function make map and map info at the same time.
	DebugNotImplemented();
}

void LevelDefinition::initCity(const MIFFile::Level &level, WEInt mifWidth, SNInt mifDepth)
{
	this->initInterior(level, mifWidth, mifDepth, nullptr);
}

void LevelDefinition::initWild(const RMDFile &rmd)
{
	DebugNotImplemented();
}

SNInt LevelDefinition::getWidth() const
{
	return this->voxels.getWidth();
}

int LevelDefinition::getHeight() const
{
	return this->voxels.getHeight();
}

WEInt LevelDefinition::getDepth() const
{
	return this->voxels.getDepth();
}

LevelDefinition::VoxelDefID LevelDefinition::getVoxel(SNInt x, int y, WEInt z) const
{
	return this->voxels.get(x, y, z);
}

void LevelDefinition::setVoxel(SNInt x, int y, WEInt z, VoxelDefID voxel)
{
	this->voxels.set(x, y, z, voxel);
}

int LevelDefinition::getEntityPlacementDefCount() const
{
	return static_cast<int>(this->entityPlacementDefs.size());
}

const LevelDefinition::EntityPlacementDef &LevelDefinition::getEntityPlacementDef(int index) const
{
	DebugAssertIndex(this->entityPlacementDefs, index);
	return this->entityPlacementDefs[index];
}

int LevelDefinition::getLockPlacementDefCount() const
{
	return static_cast<int>(this->lockPlacementDefs.size());
}

const LevelDefinition::LockPlacementDef &LevelDefinition::getLockPlacementDef(int index) const
{
	DebugAssertIndex(this->lockPlacementDefs, index);
	return this->lockPlacementDefs[index];
}

int LevelDefinition::getTriggerPlacementDefCount() const
{
	return static_cast<int>(this->triggerPlacementDefs.size());
}

const LevelDefinition::TriggerPlacementDef &LevelDefinition::getTriggerPlacementDef(int index) const
{
	DebugAssertIndex(this->triggerPlacementDefs, index);
	return this->triggerPlacementDefs[index];
}
