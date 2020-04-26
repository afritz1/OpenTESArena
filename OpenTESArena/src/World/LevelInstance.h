#ifndef LEVEL_INSTANCE_H
#define LEVEL_INSTANCE_H

#include <cstdint>
#include <vector>

#include "LevelDefinition.h"
#include "../Math/Vector3.h"

// Contains deltas and changed values for the associated level definition.

class LevelInstance
{
public:
	using ChangedVoxel = std::pair<Int3, LevelDefinition::VoxelID>;
private:
	std::vector<ChangedVoxel> changedVoxels;
	// @todo: open doors
	// @todo: fading voxels

	static Int3 makeVoxelCoord(WEInt x, int y, SNInt z);

	ChangedVoxel *findVoxel(WEInt x, int y, SNInt z);
	const ChangedVoxel *findVoxel(WEInt x, int y, SNInt z) const;
public:
	void init();

	int getChangedVoxelCount() const;
	const ChangedVoxel &getChangedVoxelAt(int index) const;

	// Checks the level instance for a changed voxel at the given coordinate, otherwise gets
	// the voxel from the level definition.
	LevelDefinition::VoxelID getVoxel(WEInt x, int y, SNInt z, const LevelDefinition &levelDef) const;

	void setChangedVoxel(WEInt x, int y, SNInt z, LevelDefinition::VoxelID voxelID);

	// @todo: update(double dt)?
};

#endif
