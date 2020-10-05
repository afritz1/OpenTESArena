#ifndef LEVEL_DEFINITION_H
#define LEVEL_DEFINITION_H

#include <cstdint>
#include <vector>

#include "VoxelUtils.h"
#include "../Assets/INFFile.h"
#include "../Assets/MIFFile.h"

#include "components/utilities/Buffer3D.h"

// A single unbaked level of a map with IDs pointing to voxels, entities, etc. defined in a level
// info definition. This can be an interior level, whole city, or wilderness block.

class ArenaRandom;
class ExeData;
class RMDFile;

class LevelDefinition
{
public:
	// Points to various definitions in a level info definition.
	using VoxelDefID = int;
	using EntityDefID = int;
	using LockDefID = int;
	using TriggerDefID = int;

	struct EntityPlacementDef
	{
		EntityDefID id;
		std::vector<LevelDouble3> positions;
	};

	struct LockPlacementDef
	{
		LockDefID id;
		std::vector<LevelInt3> positions;
	};

	struct TriggerPlacementDef
	{
		TriggerDefID id;
		std::vector<LevelInt3> positions;
	};

	// @todo: interior/city/wild structs for special data like sky color, etc.
private:
	static constexpr VoxelDefID VOXEL_ID_AIR = 0;

	Buffer3D<VoxelDefID> voxels;
	std::vector<EntityPlacementDef> entityPlacementDefs;
	std::vector<LockPlacementDef> lockPlacementDefs;
	std::vector<TriggerPlacementDef> triggerPlacementDefs;
public:
	// Initializer for an interior level with optional ceiling data.
	void initInterior(const MIFFile::Level &level, WEInt mifWidth, SNInt mifDepth,
		const INFFile::CeilingData *ceiling);

	// Initializer for a dungeon interior level with optional ceiling data. The dungeon
	// level is pieced together by multiple chunks in the base .MIF file.
	void initDungeon(ArenaRandom &random, const MIFFile &mif, int levelUpBlock,
		const int *levelDownBlock, int widthChunks, int depthChunks, SNInt gridWidth,
		WEInt gridDepth, const INFFile::CeilingData *ceiling, const ExeData &exeData);

	// Initializer for an entire city.
	void initCity(const MIFFile::Level &level, WEInt mifWidth, SNInt mifDepth);

	// Initializer for a wilderness chunk.
	void initWild(const RMDFile &rmd);

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
};

#endif
