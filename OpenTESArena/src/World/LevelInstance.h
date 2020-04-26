#ifndef LEVEL_INSTANCE_H
#define LEVEL_INSTANCE_H

#include <cstdint>
#include <vector>

#include "../Math/Vector3.h"

// Contains deltas and changed values for the associated level definition.

class LevelInstance
{
public:
	using ChangedVoxel = std::pair<Int3, uint16_t>;
private:
	std::vector<ChangedVoxel> changedVoxels;
	// @todo: open doors
	// @todo: fading voxels
public:
	void init();

	int getChangedVoxelCount() const;
	const ChangedVoxel &getChangedVoxelAt(int index) const;
	void setChangedVoxel(const Int3 &voxel, uint16_t voxelID);
};

#endif
