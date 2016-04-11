#ifndef BUILDING_TYPE_H
#define BUILDING_TYPE_H

// A unique identifier for each kind of building, both in the city and in the 
// wilderness. This does not include sprite entrances, like a den, or interiors
// themselves.

// There should be a BuildingType -> pair<VoxelType, vector<VoxelType>> mapping,
// where the pair has the doorway voxel and its associated wall voxels. All
// buildings should follow this pattern.

enum class BuildingType
{
	// Cities.
	CityGates,
	PoorHouse,
	NormalHouse,
	RichHouse,
	NobleHouse,
	Tavern,
	EquipmentStore,
	MagesGuild,
	Palace,
	Temple,

	// Wilderness.
	EvilCastle,
	Crypt,
	FarmHouse // Extrapolate "farm house" into more categories (rich, poor, etc.).
};

#endif
