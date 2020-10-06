#ifndef LEVEL_INFO_DEFINITION_H
#define LEVEL_INFO_DEFINITION_H

#include <vector>

#include "LevelDefinition.h"
#include "LockDefinition.h"
#include "TriggerDefinition.h"
#include "VoxelDefinition.h"
#include "VoxelUtils.h"
#include "../Entities/EntityDefinition.h"

// Modern replacement for .INF files; defines the actual voxels, entities, etc. pointed to by a
// level definition. This is intended to separate the level's IDs from what they're pointing to
// so it's easier to change climates, etc..

class INFFile;

class LevelInfoDefinition
{
private:
	template <typename T, typename U>
	using DefinitionList = std::vector<std::pair<T, U>>;

	// Definitions pointed to by a level definition.
	// @todo: eventually want a strictly engine-independent representation for all of these,
	// since currently voxel and entity definitions rely on runtime texture manager handles.
	// - Consider using TextureDefinition for each voxel texture/animation frame.
	DefinitionList<LevelDefinition::VoxelDefID, VoxelDefinition> voxelDefs;
	DefinitionList<LevelDefinition::EntityDefID, EntityDefinition> entityDefs;
	DefinitionList<LevelDefinition::LockDefID, LockDefinition> lockDefs;
	DefinitionList<LevelDefinition::TriggerDefID, TriggerDefinition> triggerDefs;

	double ceilingScale; // Vertical size of walls; 1.0 by default.
public:
	LevelInfoDefinition();

	void init(const INFFile &inf);

	bool tryGetVoxelDef(LevelDefinition::VoxelDefID id, const VoxelDefinition **outDef) const;
	bool tryGetEntityDef(LevelDefinition::EntityDefID id, const EntityDefinition **outDef) const;
	bool tryGetLockDef(LevelDefinition::LockDefID id, const LockDefinition **outDef) const;
	bool tryGetTriggerDef(LevelDefinition::TriggerDefID id, const TriggerDefinition **outDef) const;
	double getCeilingScale() const;
};

#endif
