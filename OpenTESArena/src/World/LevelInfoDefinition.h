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
	std::vector<VoxelTraitsDefinition> voxelTraitsDefs;
	std::vector<EntityDefinition> entityDefs;
	std::vector<LockDefinition> lockDefs;
	std::vector<VoxelTriggerDefinition> triggerDefs;
	std::vector<TransitionDefinition> transitionDefs;
	std::vector<std::string> buildingNames;
	std::unordered_map<LevelDefinition::BuildingNameID, std::string> buildingNameOverrides;
	std::vector<VoxelDoorDefinition> doorDefs;
	std::vector<VoxelChasmDefinition> chasmDefs;

	// @todo: interior gen info ID for when player creates a wall on water.

	double ceilingScale; // Vertical size of walls; 1.0 by default.
public:
	LevelInfoDefinition();

	void init(double ceilingScale);

	int getVoxelShapeDefCount() const;
	int getVoxelTextureDefCount() const;
	int getVoxelTraitsDefCount() const;
	int getEntityDefCount() const;
	int getLockDefCount() const;
	int getTriggerDefCount() const;
	int getTransitionDefCount() const;
	int getBuildingNameCount() const;
	int getDoorDefCount() const;
	int getChasmDefCount() const;

	const VoxelShapeDefinition &getVoxelShapeDef(LevelDefinition::VoxelShapeDefID id) const;
	const VoxelTextureDefinition &getVoxelTextureDef(LevelDefinition::VoxelTextureDefID id) const;
	const VoxelTraitsDefinition &getVoxelTraitsDef(LevelDefinition::VoxelTraitsDefID id) const;
	const EntityDefinition &getEntityDef(LevelDefinition::EntityDefID id) const;
	const LockDefinition &getLockDef(LevelDefinition::LockDefID id) const;
	const VoxelTriggerDefinition &getTriggerDef(LevelDefinition::TriggerDefID id) const;
	const TransitionDefinition &getTransitionDef(LevelDefinition::TransitionDefID id) const;
	const std::string &getBuildingName(LevelDefinition::BuildingNameID id) const;
	const VoxelDoorDefinition &getDoorDef(LevelDefinition::DoorDefID id) const;
	const VoxelChasmDefinition &getChasmDef(LevelDefinition::ChasmDefID id) const;
	double getCeilingScale() const;

	LevelDefinition::VoxelShapeDefID addVoxelShapeDef(VoxelShapeDefinition &&def);
	LevelDefinition::VoxelTextureDefID addVoxelTextureDef(VoxelTextureDefinition &&def);
	LevelDefinition::VoxelTraitsDefID addVoxelTraitsDef(VoxelTraitsDefinition &&def);
	LevelDefinition::EntityDefID addEntityDef(EntityDefinition &&def);
	LevelDefinition::LockDefID addLockDef(LockDefinition &&def);
	LevelDefinition::TriggerDefID addTriggerDef(VoxelTriggerDefinition &&def);
	LevelDefinition::TransitionDefID addTransitionDef(TransitionDefinition &&def);
	LevelDefinition::BuildingNameID addBuildingName(std::string &&name);
	LevelDefinition::DoorDefID addDoorDef(VoxelDoorDefinition &&def);
	LevelDefinition::ChasmDefID addChasmDef(VoxelChasmDefinition &&def);

	// Handles some special cases in main quest cities.
	void setBuildingNameOverride(LevelDefinition::BuildingNameID id, std::string &&name);
};

#endif
