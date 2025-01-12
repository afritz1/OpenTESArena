#ifndef VOXEL_TRAITS_DEFINITION_H
#define VOXEL_TRAITS_DEFINITION_H

#include "../Assets/ArenaTypes.h"

enum class VoxelFacing2D;

// Grab-bag traits that don't fit into other existing categories.
// @todo: eventually split this up into dedicated definitions
struct VoxelTraitsDefinition
{
	struct Floor
	{
		// Wild automap floor coloring to make roads, etc. easier to see.
		// @todo: maybe put in some VoxelVisibilityDefinition/VoxelAutomapTraitsDefinition?
		bool isWildWallColored;
	};

	struct TransparentWall
	{
		// @todo: maybe put in some VoxelCollisionTraitsDefinition? For other voxels, their collision def would assume 'always a collider'.
		bool collider; // Also affects automap visibility.
	};

	struct Edge
	{
		// @todo: maybe put in some VoxelCollisionTraitsDefinition?
		VoxelFacing2D facing;
		bool collider;
	};

	struct Chasm
	{
		// @todo: should move this into LevelDefinition/LevelInfoDefinition/Chunk as a VoxelChasmDefinition,
		// the same as VoxelDoorDefinition.
		ArenaTypes::ChasmType type;
	};

	// @todo: eventually this def should not depend on a voxel type; instead it should have things
	// like an interactivity enum (i.e. "is this a door?").
	ArenaTypes::VoxelType type;

	union
	{
		Floor floor;
		TransparentWall transparentWall;
		Edge edge;
		Chasm chasm;
	};

	VoxelTraitsDefinition();

	void initGeneral(ArenaTypes::VoxelType type); // @todo: ideally this function wouldn't be needed
	void initFloor(bool isWildWallColored);
	void initTransparentWall(bool collider);
	void initEdge(VoxelFacing2D facing, bool collider);
	void initChasm(ArenaTypes::ChasmType chasmType);

	bool hasCollision() const;
};

#endif
