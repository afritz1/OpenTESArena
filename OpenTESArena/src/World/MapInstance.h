#ifndef MAP_INSTANCE_H
#define MAP_INSTANCE_H

#include <vector>

#include "../Math/Vector3.h"

#include "components/utilities/Buffer.h"

// Contains deltas and changed values for the associated map definition.

class MapInstance
{
public:
	// Don't store things like chasm permutations. Unbaked voxels only.
	class Level
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
private:
	Buffer<Level> levels;
public:
	void init(int levelCount);

	Level &getLevel(int index);
};

#endif
