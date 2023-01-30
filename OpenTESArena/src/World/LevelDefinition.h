#ifndef LEVEL_DEFINITION_H
#define LEVEL_DEFINITION_H

#include <cstdint>
#include <vector>

#include "../Assets/MIFFile.h"
#include "../Voxels/VoxelUtils.h"

#include "components/utilities/Buffer3D.h"

// A single unbaked level of a map with IDs pointing to voxels, entities, etc. defined in a level
// info definition. This can be for an interior level, whole city, or wilderness block.

class LevelDefinition
{
public:
	// Points to various definitions in a level info definition.
	using VoxelMeshDefID = int;
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
		std::vector<LevelDouble3> positions;

		EntityPlacementDef(EntityDefID id, std::vector<LevelDouble3> &&positions);
	};

	struct LockPlacementDef
	{
		LockDefID id;
		std::vector<LevelInt3> positions;

		LockPlacementDef(LockDefID id, std::vector<LevelInt3> &&positions);
	};

	struct TriggerPlacementDef
	{
		TriggerDefID id;
		std::vector<LevelInt3> positions;

		TriggerPlacementDef(TriggerDefID id, std::vector<LevelInt3> &&positions);
	};

	struct TransitionPlacementDef
	{
		TransitionDefID id;
		std::vector<LevelInt3> positions; // Can also be in EntityDefinitions.

		TransitionPlacementDef(TransitionDefID id, std::vector<LevelInt3> &&positions);
	};

	struct BuildingNamePlacementDef
	{
		BuildingNameID id;
		std::vector<LevelInt3> positions;

		BuildingNamePlacementDef(BuildingNameID id, std::vector<LevelInt3> &&positions);
	};

	struct DoorPlacementDef
	{
		DoorDefID id;
		std::vector<LevelInt3> positions;

		DoorPlacementDef(DoorDefID id, std::vector<LevelInt3> &&positions);
	};

	struct ChasmPlacementDef
	{
		ChasmDefID id;
		std::vector<LevelInt3> positions;

		ChasmPlacementDef(ChasmDefID id, std::vector<LevelInt3> &&positions);
	};
private:
	Buffer3D<VoxelMeshDefID> voxelMeshIDs;
	Buffer3D<VoxelTextureDefID> voxelTextureIDs;
	Buffer3D<VoxelTraitsDefID> voxelTraitsIDs;
	std::vector<EntityPlacementDef> entityPlacementDefs;
	std::vector<LockPlacementDef> lockPlacementDefs;
	std::vector<TriggerPlacementDef> triggerPlacementDefs;
	std::vector<TransitionPlacementDef> transitionPlacementDefs;
	std::vector<BuildingNamePlacementDef> buildingNamePlacementDefs;
	std::vector<DoorPlacementDef> doorPlacementDefs;
	std::vector<ChasmPlacementDef> chasmPlacementDefs;
public:
	void init(SNInt width, int height, WEInt depth);

	SNInt getWidth() const;
	int getHeight() const;
	WEInt getDepth() const;

	VoxelMeshDefID getVoxelMeshID(SNInt x, int y, WEInt z) const;
	VoxelTextureDefID getVoxelTextureID(SNInt x, int y, WEInt z) const;
	VoxelTraitsDefID getVoxelTraitsID(SNInt x, int y, WEInt z) const;
	void setVoxelMeshID(SNInt x, int y, WEInt z, VoxelMeshDefID id);
	void setVoxelTextureID(SNInt x, int y, WEInt z, VoxelTextureDefID id);
	void setVoxelTraitsID(SNInt x, int y, WEInt z, VoxelTraitsDefID id);

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

	void addEntity(EntityDefID id, const LevelDouble3 &position);
	void addLock(LockDefID id, const LevelInt3 &position);
	void addTrigger(TriggerDefID id, const LevelInt3 &position);
	void addTransition(TransitionDefID id, const LevelInt3 &position);
	void addBuildingName(BuildingNameID id, const LevelInt3 &position);
	void addDoor(DoorDefID id, const LevelInt3 &position);
	void addChasm(ChasmDefID id, const LevelInt3 &position);
};

#endif
