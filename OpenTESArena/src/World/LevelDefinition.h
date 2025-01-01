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

// A single unbaked level of a map with IDs pointing to voxels, entities, etc. defined in a level
// info definition. This can be for an interior level, whole city, or wilderness block.
class LevelDefinition
{
public:
	struct EntityPlacementDef
	{
		LevelVoxelEntityDefID id;
		std::vector<WorldDouble3> positions;

		EntityPlacementDef(LevelVoxelEntityDefID id, std::vector<WorldDouble3> &&positions);
	};

	struct LockPlacementDef
	{
		LevelVoxelLockDefID id;
		std::vector<WorldInt3> positions;

		LockPlacementDef(LevelVoxelLockDefID id, std::vector<WorldInt3> &&positions);
	};

	struct TriggerPlacementDef
	{
		LevelVoxelTriggerDefID id;
		std::vector<WorldInt3> positions;

		TriggerPlacementDef(LevelVoxelTriggerDefID id, std::vector<WorldInt3> &&positions);
	};

	struct TransitionPlacementDef
	{
		LevelVoxelTransitionDefID id;
		std::vector<WorldInt3> positions; // Can also be in EntityDefinitions.

		TransitionPlacementDef(LevelVoxelTransitionDefID id, std::vector<WorldInt3> &&positions);
	};

	struct BuildingNamePlacementDef
	{
		LevelVoxelBuildingNameID id;
		std::vector<WorldInt3> positions;

		BuildingNamePlacementDef(LevelVoxelBuildingNameID id, std::vector<WorldInt3> &&positions);
	};

	struct DoorPlacementDef
	{
		LevelVoxelDoorDefID id;
		std::vector<WorldInt3> positions;

		DoorPlacementDef(LevelVoxelDoorDefID id, std::vector<WorldInt3> &&positions);
	};

	struct ChasmPlacementDef
	{
		LevelVoxelChasmDefID id;
		std::vector<WorldInt3> positions;

		ChasmPlacementDef(LevelVoxelChasmDefID id, std::vector<WorldInt3> &&positions);
	};
private:
	Buffer3D<LevelVoxelShapeDefID> voxelShapeIDs;
	Buffer3D<LevelVoxelTextureDefID> voxelTextureIDs;
	Buffer3D<LevelVoxelTraitsDefID> voxelTraitsIDs;
	LevelVoxelShapeDefID floorReplacementShapeDefID;
	LevelVoxelTextureDefID floorReplacementTextureDefID;
	LevelVoxelTraitsDefID floorReplacementTraitsDefID;
	LevelVoxelChasmDefID floorReplacementChasmDefID;
	std::vector<EntityPlacementDef> entityPlacementDefs;
	std::vector<LockPlacementDef> lockPlacementDefs;
	std::vector<TriggerPlacementDef> triggerPlacementDefs;
	std::vector<TransitionPlacementDef> transitionPlacementDefs;
	std::vector<BuildingNamePlacementDef> buildingNamePlacementDefs;
	std::vector<DoorPlacementDef> doorPlacementDefs;
	std::vector<ChasmPlacementDef> chasmPlacementDefs;
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
	const EntityPlacementDef &getEntityPlacementDef(int index) const;
	int getLockPlacementDefCount() const;
	const LockPlacementDef &getLockPlacementDef(int index) const;
	int getTriggerPlacementDefCount() const;
	const TriggerPlacementDef &getTriggerPlacementDef(int index) const;
	int getTransitionPlacementDefCount() const;
	const TransitionPlacementDef &getTransitionPlacementDef(int index) const;
	int getBuildingNamePlacementDefCount() const;
	const BuildingNamePlacementDef &getBuildingNamePlacementDef(int index) const;
	int getDoorPlacementDefCount() const;
	const DoorPlacementDef &getDoorPlacementDef(int index) const;
	int getChasmPlacementDefCount() const;
	const ChasmPlacementDef &getChasmPlacementDef(int index) const;

	void addEntity(LevelVoxelEntityDefID id, const WorldDouble3 &position);
	void addLock(LevelVoxelLockDefID id, const WorldInt3 &position);
	void addTrigger(LevelVoxelTriggerDefID id, const WorldInt3 &position);
	void addTransition(LevelVoxelTransitionDefID id, const WorldInt3 &position);
	void addBuildingName(LevelVoxelBuildingNameID id, const WorldInt3 &position);
	void addDoor(LevelVoxelDoorDefID id, const WorldInt3 &position);
	void addChasm(LevelVoxelChasmDefID id, const WorldInt3 &position);
};

#endif
