#ifndef VOXEL_DEFINITION_H
#define VOXEL_DEFINITION_H

#include "../Assets/ArenaTypes.h"
#include "../Assets/TextureAsset.h"

#include "components/utilities/Buffer.h"

// The definition that a voxel ID points to, used for rendering, collision detection, and coloring automap voxels.

enum class VoxelFacing2D;
enum class VoxelType;

class VoxelDefinition
{
public:
	// @todo: to be more data-driven, all structs here could be changed to lists of rectangles with texture asset references.
	// - think of each struct as implicitly defining a set of rectangles that are calculated elsewhere (which is bad/hardcoded!).

	// Regular wall with height equal to ceiling height.
	struct WallData
	{
		TextureAsset sideTextureAsset, floorTextureAsset, ceilingTextureAsset;

		void init(TextureAsset &&sideTextureAsset, TextureAsset &&floorTextureAsset,
			TextureAsset &&ceilingTextureAsset);
	};

	// Floors only have their top rendered.
	struct FloorData
	{
		TextureAsset textureAsset;

		// Wild automap floor coloring to make roads, etc. easier to see.
		bool isWildWallColored;

		void init(TextureAsset &&textureAsset, bool isWildWallColored);
	};

	// Ceilings only have their bottom rendered.
	struct CeilingData
	{
		TextureAsset textureAsset;

		void init(TextureAsset &&textureAsset);
	};

	// Raised platform at some Y offset in the voxel.
	struct RaisedData
	{
		TextureAsset sideTextureAsset, floorTextureAsset, ceilingTextureAsset;
		double yOffset, ySize, vTop, vBottom;

		void init(TextureAsset &&sideTextureAsset, TextureAsset &&floorTextureAsset,
			TextureAsset &&ceilingTextureAsset, double yOffset, double ySize, double vTop,
			double vBottom);
	};

	// Diagonal wall with variable start and end corners.
	struct DiagonalData
	{
		TextureAsset textureAsset;
		bool type1; // Type 1 is '/', (nearX, nearZ) -> (farX, farZ).

		void init(TextureAsset &&textureAsset, bool type1);
	};

	// Transparent walls only shows front-facing textures (wooden arches, hedges, etc.).
	// Nothing is drawn when the player is in the same voxel.
	struct TransparentWallData
	{
		TextureAsset textureAsset;
		bool collider; // Also affects automap visibility.

		void init(TextureAsset &&textureAsset, bool collider);
	};

	// Rendered on one edge of a voxel with height equal to ceiling height. The facing determines
	// which side the edge is on.
	struct EdgeData
	{
		TextureAsset textureAsset;
		double yOffset;
		bool collider;

		// Not present in original game; necessary for all texture coordinates to be correct,
		// i.e., both palace graphics and store signs.
		bool flipped;

		VoxelFacing2D facing;

		void init(TextureAsset &&textureAsset, double yOffset, bool collider, bool flipped,
			VoxelFacing2D facing);
	};

	// Chasms have zero to four wall faces (stored with voxel instance) depending on adjacent floors.
	// Each face is front-facing and back-facing.
	struct ChasmData
	{
		TextureAsset textureAsset;

		// @todo: should move this into LevelDefinition/LevelInfoDefinition/Chunk as a ChasmDefinition,
		// the same as DoorDefinition.
		ArenaTypes::ChasmType type;

		void init(TextureAsset &&textureAsset, ArenaTypes::ChasmType type);

		bool matches(const ChasmData &other) const;
	};

	struct DoorData
	{
		TextureAsset textureAsset;
		
		// @todo: DoorDefinition has effectively replaced this. Just need VoxelDefinition to become a geometry
		// container and all of the VoxelGeometry and SoftwareRenderer dependencies on this removed too.
		ArenaTypes::DoorType type;

		void init(TextureAsset &&textureAsset, ArenaTypes::DoorType type);
	};

	// Determines how the voxel definition is accessed.
	ArenaTypes::VoxelType type;

	// Only one voxel type can be active at a time, given by "type". No longer a union due to the
	// added complexity of texture asset references.
	WallData wall;
	FloorData floor;
	CeilingData ceiling;
	RaisedData raised;
	DiagonalData diagonal;
	TransparentWallData transparentWall;
	EdgeData edge;
	ChasmData chasm;
	DoorData door;

	VoxelDefinition();

	static VoxelDefinition makeWall(TextureAsset &&sideTextureAsset,
		TextureAsset &&floorTextureAsset, TextureAsset &&ceilingTextureAsset);
	static VoxelDefinition makeFloor(TextureAsset &&textureAsset, bool isWildWallColored);
	static VoxelDefinition makeCeiling(TextureAsset &&textureAsset);
	static VoxelDefinition makeRaised(TextureAsset &&sideTextureAsset,
		TextureAsset &&floorTextureAsset, TextureAsset &&ceilingTextureAsset,
		double yOffset, double ySize, double vTop, double vBottom);
	static VoxelDefinition makeDiagonal(TextureAsset &&textureAsset, bool type1);
	static VoxelDefinition makeTransparentWall(TextureAsset &&textureAsset, bool collider);
	static VoxelDefinition makeEdge(TextureAsset &&textureAsset, double yOffset, bool collider,
		bool flipped, VoxelFacing2D facing);
	static VoxelDefinition makeChasm(TextureAsset &&textureAsset, ArenaTypes::ChasmType type);
	static VoxelDefinition makeDoor(TextureAsset &&textureAsset, ArenaTypes::DoorType type);

	// Whether this voxel definition contributes to a chasm having a wall face.
	bool allowsChasmFace() const;

	// Gets the texture asset references and count from the voxel definition based on its type.
	int getTextureAssetCount() const;
	const TextureAsset &getTextureAsset(int index) const;
};

#endif
