#ifndef LEVEL_INFO_DEFINITION_H
#define LEVEL_INFO_DEFINITION_H

#include <string>
#include <unordered_map>
#include <vector>

#include "LevelDefinition.h"
#include "LockDefinition.h"
#include "MapGeneration.h"
#include "TransitionDefinition.h"
#include "../Entities/EntityDefinition.h"
#include "../Voxels/VoxelChasmDefinition.h"
#include "../Voxels/VoxelDoorDefinition.h"
#include "../Voxels/VoxelShadingDefinition.h"
#include "../Voxels/VoxelShapeDefinition.h"
#include "../Voxels/VoxelTextureDefinition.h"
#include "../Voxels/VoxelTraitsDefinition.h"
#include "../Voxels/VoxelTriggerDefinition.h"
#include "../Voxels/VoxelUtils.h"

// Modern replacement for .INF files; defines the actual voxels, entities, etc. pointed to by a
// level definition. This is intended to separate the level's IDs from what they're pointing to
// so it's easier to change climates, etc..
class LevelInfoDefinition
{
private:
	// Definitions pointed to by a level definition. These should all be engine-independent now
	// (meaning that they could theoretically work with a standalone editor).
	std::vector<VoxelShapeDefinition> voxelShapeDefs;
	std::vector<VoxelTextureDefinition> voxelTextureDefs;
	std::vector<VoxelShadingDefinition> voxelShadingDefs;
	std::vector<VoxelTraitsDefinition> voxelTraitsDefs;
	std::vector<EntityDefinition> entityDefs;
	std::vector<LockDefinition> lockDefs;
	std::vector<VoxelTriggerDefinition> triggerDefs;
	std::vector<TransitionDefinition> transitionDefs;
	std::vector<std::string> buildingNames;
	std::unordered_map<LevelVoxelBuildingNameID, std::string> buildingNameOverrides;
	std::vector<VoxelDoorDefinition> doorDefs;
	std::vector<VoxelChasmDefinition> chasmDefs;

	// @todo: interior gen info ID for when player creates a wall on water.

	double ceilingScale; // Vertical size of walls; 1.0 by default.
public:
	LevelInfoDefinition();

	void init(double ceilingScale);

	int getVoxelShapeDefCount() const;
	int getVoxelTextureDefCount() const;
	int getVoxelShadingDefCount() const;
	int getVoxelTraitsDefCount() const;
	int getEntityDefCount() const;
	int getLockDefCount() const;
	int getTriggerDefCount() const;
	int getTransitionDefCount() const;
	int getBuildingNameCount() const;
	int getDoorDefCount() const;
	int getChasmDefCount() const;

	const VoxelShapeDefinition &getVoxelShapeDef(LevelVoxelShapeDefID id) const;
	const VoxelTextureDefinition &getVoxelTextureDef(LevelVoxelTextureDefID id) const;
	const VoxelShadingDefinition &getVoxelShadingDef(LevelVoxelShadingDefID id) const;
	const VoxelTraitsDefinition &getVoxelTraitsDef(LevelVoxelTraitsDefID id) const;
	const EntityDefinition &getEntityDef(LevelVoxelEntityDefID id) const;
	const LockDefinition &getLockDef(LevelVoxelLockDefID id) const;
	const VoxelTriggerDefinition &getTriggerDef(LevelVoxelTriggerDefID id) const;
	const TransitionDefinition &getTransitionDef(LevelVoxelTransitionDefID id) const;
	const std::string &getBuildingName(LevelVoxelBuildingNameID id) const;
	const VoxelDoorDefinition &getDoorDef(LevelVoxelDoorDefID id) const;
	const VoxelChasmDefinition &getChasmDef(LevelVoxelChasmDefID id) const;
	double getCeilingScale() const;

	LevelVoxelShapeDefID addVoxelShapeDef(VoxelShapeDefinition &&def);
	LevelVoxelTextureDefID addVoxelTextureDef(VoxelTextureDefinition &&def);
	LevelVoxelShadingDefID addVoxelShadingDef(VoxelShadingDefinition &&def);
	LevelVoxelTraitsDefID addVoxelTraitsDef(VoxelTraitsDefinition &&def);
	LevelVoxelEntityDefID addEntityDef(EntityDefinition &&def);
	LevelVoxelLockDefID addLockDef(LockDefinition &&def);
	LevelVoxelTriggerDefID addTriggerDef(VoxelTriggerDefinition &&def);
	LevelVoxelTransitionDefID addTransitionDef(TransitionDefinition &&def);
	LevelVoxelBuildingNameID addBuildingName(std::string &&name);
	LevelVoxelDoorDefID addDoorDef(VoxelDoorDefinition &&def);
	LevelVoxelChasmDefID addChasmDef(VoxelChasmDefinition &&def);

	// Handles some special cases where multiple locks are defined for the same voxel.
	void setLockLevel(LevelVoxelLockDefID id, int lockLevel);

	// Applied after transition and building name generation due to circular dependency. Used with dialogue.
	void setTransitionInteriorDisplayName(LevelVoxelTransitionDefID id, std::string &&name);

	// Handles some special cases in main quest cities.
	void setBuildingNameOverride(LevelVoxelBuildingNameID id, std::string &&name);
};

#endif
