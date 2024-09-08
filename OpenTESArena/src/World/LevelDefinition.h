#ifndef LEVEL_DEFINITION_H
#define LEVEL_DEFINITION_H

#include <cstdint>
#include <vector>

#include "../Assets/INFFile.h"
#include "../Assets/MIFFile.h"
#include "../Voxels/VoxelUtils.h"

#include "components/utilities/Buffer3D.h"

// A single unbaked level of a map with IDs pointing to voxels, entities, etc. defined in a level
// info definition. This can be for an interior level, whole city, or wilderness block.

class LevelDefinition
{
public:
	// Points to various definitions in a level info definition.
	using VoxelShapeDefID = int;
	using VoxelTextureDefID = int;
	using VoxelTraitsDefID = int;
	using EntityDefID = int;
	using LockDefID = int;
	using TriggerDefID = int;
	using BuildingNameID = int;
	using TransitionDefID = int;
	using DoorDefID = int;
	using ChasmDefID = int;

	struct EntityPlacementDef
	{
		EntityDefID id;
		std::vector<WorldDouble3> positions;

		EntityPlacementDef(EntityDefID id, std::vector<WorldDouble3> &&positions);
	};

	struct LockPlacementDef
	{
		LockDefID id;
		std::vector<WorldInt3> positions;

		LockPlacementDef(LockDefID id, std::vector<WorldInt3> &&positions);
	};

	struct TriggerPlacementDef
	{
		TriggerDefID id;
		std::vector<WorldInt3> positions;

		TriggerPlacementDef(TriggerDefID id, std::vector<WorldInt3> &&positions);
	};

	struct TransitionPlacementDef
	{
		TransitionDefID id;
		std::vector<WorldInt3> positions; // Can also be in EntityDefinitions.

		TransitionPlacementDef(TransitionDefID id, std::vector<WorldInt3> &&positions);
	};

	struct BuildingNamePlacementDef
	{
		BuildingNameID id;
		std::vector<WorldInt3> positions;

		BuildingNamePlacementDef(BuildingNameID id, std::vector<WorldInt3> &&positions);
	};

	struct DoorPlacementDef
	{
		DoorDefID id;
		std::vector<WorldInt3> positions;

		DoorPlacementDef(DoorDefID id, std::vector<WorldInt3> &&positions);
	};

	struct ChasmPlacementDef
	{
		ChasmDefID id;
		std::vector<WorldInt3> positions;

		ChasmPlacementDef(ChasmDefID id, std::vector<WorldInt3> &&positions);
	};
private:
	Buffer3D<VoxelShapeDefID> voxelShapeIDs;
	Buffer3D<VoxelTextureDefID> voxelTextureIDs;
	Buffer3D<VoxelTraitsDefID> voxelTraitsIDs;
	VoxelShapeDefID floorReplacementShapeDefID;
	VoxelTextureDefID floorReplacementTextureDefID;
	VoxelTraitsDefID floorReplacementTraitsDefID;
	ChasmDefID floorReplacementChasmDefID;
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

	VoxelShapeDefID getVoxelShapeID(SNInt x, int y, WEInt z) const;
	VoxelTextureDefID getVoxelTextureID(SNInt x, int y, WEInt z) const;
	VoxelTraitsDefID getVoxelTraitsID(SNInt x, int y, WEInt z) const;
	void setVoxelShapeID(SNInt x, int y, WEInt z, VoxelShapeDefID id);
	void setVoxelTextureID(SNInt x, int y, WEInt z, VoxelTextureDefID id);
	void setVoxelTraitsID(SNInt x, int y, WEInt z, VoxelTraitsDefID id);

	VoxelShapeDefID getFloorReplacementShapeDefID() const;
	VoxelTextureDefID getFloorReplacementTextureDefID() const;
	VoxelTraitsDefID getFloorReplacementTraitsDefID() const;
	ChasmDefID getFloorReplacementChasmDefID() const;
	void setFloorReplacementShapeDefID(VoxelShapeDefID id);
	void setFloorReplacementTextureDefID(VoxelTextureDefID id);
	void setFloorReplacementTraitsDefID(VoxelTraitsDefID id);
	void setFloorReplacementChasmDefID(ChasmDefID id);

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

	void addEntity(EntityDefID id, const WorldDouble3 &position);
	void addLock(LockDefID id, const WorldInt3 &position);
	void addTrigger(TriggerDefID id, const WorldInt3 &position);
	void addTransition(TransitionDefID id, const WorldInt3 &position);
	void addBuildingName(BuildingNameID id, const WorldInt3 &position);
	void addDoor(DoorDefID id, const WorldInt3 &position);
	void addChasm(ChasmDefID id, const WorldInt3 &position);
};

#endif
