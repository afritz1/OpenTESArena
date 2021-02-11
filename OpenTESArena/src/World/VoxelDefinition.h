#ifndef VOXEL_DEFINITION_H
#define VOXEL_DEFINITION_H

#include "../Assets/ArenaTypes.h"
#include "../Assets/TextureAssetReference.h"

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
		TextureAssetReference sideTextureAssetRef, floorTextureAssetRef, ceilingTextureAssetRef;

		void init(TextureAssetReference &&sideTextureAssetRef, TextureAssetReference &&floorTextureAssetRef,
			TextureAssetReference &&ceilingTextureAssetRef);
	};

	// Floors only have their top rendered.
	struct FloorData
	{
		TextureAssetReference textureAssetRef;

		// Wild automap floor coloring to make roads, etc. easier to see.
		bool isWildWallColored;

		void init(TextureAssetReference &&textureAssetRef, bool isWildWallColored);
	};

	// Ceilings only have their bottom rendered.
	struct CeilingData
	{
		TextureAssetReference textureAssetRef;

		void init(TextureAssetReference &&textureAssetRef);
	};

	// Raised platform at some Y offset in the voxel.
	struct RaisedData
	{
		TextureAssetReference sideTextureAssetRef, floorTextureAssetRef, ceilingTextureAssetRef;
		double yOffset, ySize, vTop, vBottom;

		void init(TextureAssetReference &&sideTextureAssetRef, TextureAssetReference &&floorTextureAssetRef,
			TextureAssetReference &&ceilingTextureAssetRef, double yOffset, double ySize, double vTop,
			double vBottom);
	};

	// Diagonal wall with variable start and end corners.
	struct DiagonalData
	{
		TextureAssetReference textureAssetRef;
		bool type1; // Type 1 is '/', (nearX, nearZ) -> (farX, farZ).

		void init(TextureAssetReference &&textureAssetRef, bool type1);
	};

	// Transparent walls only shows front-facing textures (wooden arches, hedges, etc.).
	// Nothing is drawn when the player is in the same voxel.
	struct TransparentWallData
	{
		TextureAssetReference textureAssetRef;
		bool collider; // Also affects automap visibility.

		void init(TextureAssetReference &&textureAssetRef, bool collider);
	};

	// Rendered on one edge of a voxel with height equal to ceiling height. The facing determines
	// which side the edge is on.
	struct EdgeData
	{
		TextureAssetReference textureAssetRef;
		double yOffset;
		bool collider;

		// Not present in original game; necessary for all texture coordinates to be correct,
		// i.e., both palace graphics and store signs.
		bool flipped;

		VoxelFacing2D facing;

		void init(TextureAssetReference &&textureAssetRef, double yOffset, bool collider, bool flipped,
			VoxelFacing2D facing);
	};

	// Chasms have zero to four wall faces (stored with voxel instance) depending on adjacent floors.
	// Each face is front-facing and back-facing.
	struct ChasmData
	{
		TextureAssetReference textureAssetRef;
		ArenaTypes::ChasmType type;

		void init(TextureAssetReference &&textureAssetRef, ArenaTypes::ChasmType type);

		bool matches(const ChasmData &other) const;
	};

	struct DoorData
	{
		TextureAssetReference textureAssetRef;
		ArenaTypes::DoorType type;

		void init(TextureAssetReference &&textureAssetRef, ArenaTypes::DoorType type);
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

	static VoxelDefinition makeWall(TextureAssetReference &&sideTextureAssetRef,
		TextureAssetReference &&floorTextureAssetRef, TextureAssetReference &&ceilingTextureAssetRef);
	static VoxelDefinition makeFloor(TextureAssetReference &&textureAssetRef, bool isWildWallColored);
	static VoxelDefinition makeCeiling(TextureAssetReference &&textureAssetRef);
	static VoxelDefinition makeRaised(TextureAssetReference &&sideTextureAssetRef,
		TextureAssetReference &&floorTextureAssetRef, TextureAssetReference &&ceilingTextureAssetRef,
		double yOffset, double ySize, double vTop, double vBottom);
	static VoxelDefinition makeDiagonal(TextureAssetReference &&textureAssetRef, bool type1);
	static VoxelDefinition makeTransparentWall(TextureAssetReference &&textureAssetRef, bool collider);
	static VoxelDefinition makeEdge(TextureAssetReference &&textureAssetRef, double yOffset, bool collider,
		bool flipped, VoxelFacing2D facing);
	static VoxelDefinition makeChasm(TextureAssetReference &&textureAssetRef, ArenaTypes::ChasmType type);
	static VoxelDefinition makeDoor(TextureAssetReference &&textureAssetRef, ArenaTypes::DoorType type);

	// Whether this voxel definition contributes to a chasm having a wall face.
	bool allowsChasmFace() const;

	// Gets all the texture asset references from the voxel definition based on its type.
	Buffer<TextureAssetReference> getTextureAssetReferences() const;
};

#endif
