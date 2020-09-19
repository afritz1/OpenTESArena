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

// A single unbaked level of a map with voxels, entities, etc.. It can be an interior level,
// whole city, or wilderness block.

class LevelDefinition
{
public:
	// 2 bytes per voxel because the map might be bigger than a chunk.
	using VoxelID = uint16_t;
private:
	struct EntityPlacementDef
	{
		int defsIndex; // Index into entity definitions list.
		std::vector<LevelDouble2> positions;
	};

	Buffer3D<VoxelID> voxels; // Points into voxel definitions list.
	std::vector<VoxelDefinition> voxelDefs;
	std::vector<EntityPlacementDef> entityPlacementDefs; // Points into entity definitions list.
	std::vector<EntityDefinition> entityDefs;
	std::vector<LockDefinition> lockDefs;
	std::vector<TriggerDefinition> triggerDefs;
public:
	static constexpr VoxelID AIR = 0;

	void init(SNInt width, int height, WEInt depth);

	// Width and depth are the same as the map the level is in.
	SNInt getWidth() const;
	int getHeight() const;
	WEInt getDepth() const;

	int getVoxelDefCount() const;
	const VoxelDefinition &getVoxelDef(VoxelID id) const;

	int getEntityPlacementDefCount() const;
	const EntityPlacementDef &getEntityPlacementDef(int index) const;

	int getEntityDefsCount() const;
	const EntityDefinition &getEntityDef(int index) const;

	int getLockDefsCount() const;
	const LockDefinition &getLockDef(int index) const;

	int getTriggerDefCount() const;
	const TriggerDefinition &getTriggerDef(int index) const;

	VoxelID getVoxel(SNInt x, int y, WEInt z) const;
	void setVoxel(SNInt x, int y, WEInt z, VoxelID voxel);
};

#endif
