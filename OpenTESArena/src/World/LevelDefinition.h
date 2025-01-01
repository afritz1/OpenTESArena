#ifndef LEVEL_DEFINITION_H
#define LEVEL_DEFINITION_H

#include <cstdint>
#include <vector>

#include "../Assets/INFFile.h"
#include "../Assets/MIFFile.h"
#include "../Voxels/VoxelUtils.h"

#include "components/utilities/Buffer3D.h"

// Points to various definitions in a level info definition.
using LevelVoxelShapeDefID = int;
using LevelVoxelTextureDefID = int;
using LevelVoxelTraitsDefID = int;
using LevelVoxelEntityDefID = int;
using LevelVoxelLockDefID = int;
using LevelVoxelTriggerDefID = int;
using LevelVoxelBuildingNameID = int;
using LevelVoxelTransitionDefID = int;
using LevelVoxelDoorDefID = int;
using LevelVoxelChasmDefID = int;

struct LevelEntityPlacementDefinition
{
	LevelVoxelEntityDefID id;
	std::vector<WorldDouble3> positions;

	LevelEntityPlacementDefinition(LevelVoxelEntityDefID id, std::vector<WorldDouble3> &&positions);
};

struct LevelLockPlacementDefinition
{
	LevelVoxelLockDefID id;
	std::vector<WorldInt3> positions;

	LevelLockPlacementDefinition(LevelVoxelLockDefID id, std::vector<WorldInt3> &&positions);
};

struct LevelTriggerPlacementDefinition
{
	LevelVoxelTriggerDefID id;
	std::vector<WorldInt3> positions;

	LevelTriggerPlacementDefinition(LevelVoxelTriggerDefID id, std::vector<WorldInt3> &&positions);
};

struct LevelTransitionPlacementDefinition
{
	LevelVoxelTransitionDefID id;
	std::vector<WorldInt3> positions; // Can also be in EntityDefinitions.

	LevelTransitionPlacementDefinition(LevelVoxelTransitionDefID id, std::vector<WorldInt3> &&positions);
};

struct LevelBuildingNamePlacementDefinition
{
	LevelVoxelBuildingNameID id;
	std::vector<WorldInt3> positions;

	LevelBuildingNamePlacementDefinition(LevelVoxelBuildingNameID id, std::vector<WorldInt3> &&positions);
};

struct LevelDoorPlacementDefinition
{
	LevelVoxelDoorDefID id;
	std::vector<WorldInt3> positions;

	LevelDoorPlacementDefinition(LevelVoxelDoorDefID id, std::vector<WorldInt3> &&positions);
};

struct LevelChasmPlacementDefinition
{
	LevelVoxelChasmDefID id;
	std::vector<WorldInt3> positions;

	LevelChasmPlacementDefinition(LevelVoxelChasmDefID id, std::vector<WorldInt3> &&positions);
};

// A single unbaked level of a map with IDs pointing to voxels, entities, etc. defined in a level
// info definition. This can be for an interior level, whole city, or wilderness block.
class LevelDefinition
{
private:
	Buffer3D<LevelVoxelShapeDefID> voxelShapeIDs;
	Buffer3D<LevelVoxelTextureDefID> voxelTextureIDs;
	Buffer3D<LevelVoxelTraitsDefID> voxelTraitsIDs;
	LevelVoxelShapeDefID floorReplacementShapeDefID;
	LevelVoxelTextureDefID floorReplacementTextureDefID;
	LevelVoxelTraitsDefID floorReplacementTraitsDefID;
	LevelVoxelChasmDefID floorReplacementChasmDefID;
	std::vector<LevelEntityPlacementDefinition> entityPlacementDefs;
	std::vector<LevelLockPlacementDefinition> lockPlacementDefs;
	std::vector<LevelTriggerPlacementDefinition> triggerPlacementDefs;
	std::vector<LevelTransitionPlacementDefinition> transitionPlacementDefs;
	std::vector<LevelBuildingNamePlacementDefinition> buildingNamePlacementDefs;
	std::vector<LevelDoorPlacementDefinition> doorPlacementDefs;
	std::vector<LevelChasmPlacementDefinition> chasmPlacementDefs;
public:
	LevelDefinition();

	void init(SNInt width, int height, WEInt depth);

	SNInt getWidth() const;
	int getHeight() const;
	WEInt getDepth() const;

	LevelVoxelShapeDefID getVoxelShapeID(SNInt x, int y, WEInt z) const;
	LevelVoxelTextureDefID getVoxelTextureID(SNInt x, int y, WEInt z) const;
	LevelVoxelTraitsDefID getVoxelTraitsID(SNInt x, int y, WEInt z) const;
	void setVoxelShapeID(SNInt x, int y, WEInt z, LevelVoxelShapeDefID id);
	void setVoxelTextureID(SNInt x, int y, WEInt z, LevelVoxelTextureDefID id);
	void setVoxelTraitsID(SNInt x, int y, WEInt z, LevelVoxelTraitsDefID id);

	LevelVoxelShapeDefID getFloorReplacementShapeDefID() const;
	LevelVoxelTextureDefID getFloorReplacementTextureDefID() const;
	LevelVoxelTraitsDefID getFloorReplacementTraitsDefID() const;
	LevelVoxelChasmDefID getFloorReplacementChasmDefID() const;
	void setFloorReplacementShapeDefID(LevelVoxelShapeDefID id);
	void setFloorReplacementTextureDefID(LevelVoxelTextureDefID id);
	void setFloorReplacementTraitsDefID(LevelVoxelTraitsDefID id);
	void setFloorReplacementChasmDefID(LevelVoxelChasmDefID id);

	int getEntityPlacementDefCount() const;
	const LevelEntityPlacementDefinition &getEntityPlacementDef(int index) const;
	int getLockPlacementDefCount() const;
	const LevelLockPlacementDefinition &getLockPlacementDef(int index) const;
	int getTriggerPlacementDefCount() const;
	const LevelTriggerPlacementDefinition &getTriggerPlacementDef(int index) const;
	int getTransitionPlacementDefCount() const;
	const LevelTransitionPlacementDefinition &getTransitionPlacementDef(int index) const;
	int getBuildingNamePlacementDefCount() const;
	const LevelBuildingNamePlacementDefinition &getBuildingNamePlacementDef(int index) const;
	int getDoorPlacementDefCount() const;
	const LevelDoorPlacementDefinition &getDoorPlacementDef(int index) const;
	int getChasmPlacementDefCount() const;
	const LevelChasmPlacementDefinition &getChasmPlacementDef(int index) const;

	void addEntity(LevelVoxelEntityDefID id, const WorldDouble3 &position);
	void addLock(LevelVoxelLockDefID id, const WorldInt3 &position);
	void addTrigger(LevelVoxelTriggerDefID id, const WorldInt3 &position);
	void addTransition(LevelVoxelTransitionDefID id, const WorldInt3 &position);
	void addBuildingName(LevelVoxelBuildingNameID id, const WorldInt3 &position);
	void addDoor(LevelVoxelDoorDefID id, const WorldInt3 &position);
	void addChasm(LevelVoxelChasmDefID id, const WorldInt3 &position);
};

#endif
