#ifndef LEVEL_DEFINITION_H
#define LEVEL_DEFINITION_H

#include <cstdint>
#include <vector>

#include "LockDefinition.h"
#include "TriggerDefinition.h"
#include "VoxelDefinition.h"
#include "VoxelUtils.h"
#include "../Entities/EntityDefinition.h"

#include "components/utilities/Buffer3D.h"

// A single level of a map with voxels, entities, etc..

class LevelDefinition
{
public:
	// 2 bytes per voxel because the map might be bigger than a chunk.
	using VoxelID = uint16_t;
private:
	struct EntityPlacementDef
	{
		int defsIndex; // Index into entity definitions list.
		std::vector<OriginalDouble2> positions;
	};

	Buffer3D<VoxelID> voxels; // Points into voxel definitions list.
	std::vector<VoxelDefinition> voxelDefs;
	std::vector<EntityPlacementDef> entityPlacementDefs; // Points into entity definitions list.
	std::vector<EntityDefinition> entityDefs;
	std::vector<LockDefinition> lockDefs;
	std::vector<TriggerDefinition> triggerDefs;
public:
	void init(WEInt width, int height, SNInt depth);

	// Width and depth are the same as the map the level is in.
	WEInt getWidth() const;
	int getHeight() const;
	SNInt getDepth() const;

	VoxelID getVoxel(WEInt x, int y, SNInt z) const;
	void setVoxel(WEInt x, int y, SNInt z, VoxelID voxel);
};

#endif
