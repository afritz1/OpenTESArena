#ifndef LEVEL_DEFINITION_H
#define LEVEL_DEFINITION_H

#include <cstdint>
#include <vector>

#include "VoxelUtils.h"
#include "../Assets/INFFile.h"
#include "../Assets/MIFFile.h"

#include "components/utilities/Buffer3D.h"

// A single unbaked level of a map with IDs pointing to voxels, entities, etc. defined in a level
// info definition. This can be for an interior level, whole city, or wilderness block.

class LevelDefinition
{
public:
	// Points to various definitions in a level info definition.
	using VoxelDefID = int;
	using EntityDefID = int;
	using LockDefID = int;
	using TriggerDefID = int;
	using BuildingNameID = int;
	using TransitionDefID = int;

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
private:
	Buffer3D<VoxelDefID> voxels;
	std::vector<EntityPlacementDef> entityPlacementDefs;
	std::vector<LockPlacementDef> lockPlacementDefs;
	std::vector<TriggerPlacementDef> triggerPlacementDefs;
	std::vector<TransitionPlacementDef> transitionPlacementDefs;
	std::vector<BuildingNamePlacementDef> buildingNamePlacementDefs;
public:
	void init(SNInt width, int height, WEInt depth);

	SNInt getWidth() const;
	int getHeight() const;
	WEInt getDepth() const;

	VoxelDefID getVoxel(SNInt x, int y, WEInt z) const;
	void setVoxel(SNInt x, int y, WEInt z, VoxelDefID voxel);

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

	void addEntity(EntityDefID id, const LevelDouble3 &position);
	void addLock(LockDefID id, const LevelInt3 &position);
	void addTrigger(TriggerDefID id, const LevelInt3 &position);
	void addTransition(TransitionDefID id, const LevelInt3 &position);
	void addBuildingName(BuildingNameID id, const LevelInt3 &position);
};

#endif
