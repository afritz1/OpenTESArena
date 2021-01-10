#ifndef VOXEL_DEFINITION_H
#define VOXEL_DEFINITION_H

#include "../Media/TextureUtils.h"

// The definition of a voxel that a voxel ID points to. Since there will only be a few kinds
// of voxels per world, their size can be much larger than just a byte or two.

// A voxel's definition is used for several things, such as rendering, collision detection,
// and coloring on the automap.

enum class MapType;
enum class VoxelFacing2D;
enum class VoxelType;

class VoxelDefinition
{
public:
	// Regular wall with height equal to ceiling height.
	struct WallData
	{
		int sideID, floorID, ceilingID;

		void init(int sideID, int floorID, int ceilingID);
	};

	// Floors only have their top rendered.
	struct FloorData
	{
		int id;

		// Wild automap floor coloring to make roads, etc. easier to see.
		bool isWildWallColored;

		void init(int id, bool isWildWallColored);
	};

	// Ceilings only have their bottom rendered.
	struct CeilingData
	{
		int id;

		void init(int id);
	};

	// Raised platform at some Y offset in the voxel.
	struct RaisedData
	{
		int sideID, floorID, ceilingID;
		double yOffset, ySize, vTop, vBottom;

		void init(int sideID, int floorID, int ceilingID, double yOffset, double ySize,
			double vTop, double vBottom);
	};

	// Diagonal wall with variable start and end corners.
	struct DiagonalData
	{
		int id;
		bool type1; // Type 1 is '/', (nearX, nearZ) -> (farX, farZ).

		void init(int id, bool type1);
	};

	// Transparent walls only shows front-facing textures (wooden arches, hedges, etc.).
	// Nothing is drawn when the player is in the same voxel.
	struct TransparentWallData
	{
		int id;
		bool collider; // Also affects automap visibility.

		void init(int id, bool collider);
	};

	// Rendered on one edge of a voxel with height equal to ceiling height. The facing determines
	// which side the edge is on.
	struct EdgeData
	{
		int id;
		double yOffset;
		bool collider;

		// Not present in original game; necessary for all texture coordinates to be correct,
		// i.e., both palace graphics and store signs.
		bool flipped;

		VoxelFacing2D facing;

		void init(int id, double yOffset, bool collider, bool flipped, VoxelFacing2D facing);
	};

	// Chasms have zero to four wall faces (stored with voxel instance) depending on adjacent floors.
	// Each face is front-facing and back-facing.
	struct ChasmData
	{
		enum class Type { Dry, Wet, Lava };

		int id;
		Type type;

		void init(int id, Type type);

		bool matches(const ChasmData &other) const;
	};

	struct DoorData
	{
		// Each type of door. Most doors swing open, while others raise up or slide to the side.
		// Splitting doors are unused in the original game.
		enum class Type { Swinging, Sliding, Raising, Splitting };

		int id;
		Type type;

		void init(int id, Type type);
	};

	VoxelType type; // Defines how the voxel is interpreted and rendered.

	// Only one voxel data type can be active at a time, given by "type".
	union
	{
		WallData wall;
		FloorData floor;
		CeilingData ceiling;
		RaisedData raised;
		DiagonalData diagonal;
		TransparentWallData transparentWall;
		EdgeData edge;
		ChasmData chasm;
		DoorData door;
	};

	VoxelDefinition();

	static VoxelDefinition makeWall(int sideID, int floorID, int ceilingID);
	static VoxelDefinition makeFloor(int id, bool isWildWallColored);
	static VoxelDefinition makeCeiling(int id);
	static VoxelDefinition makeRaised(int sideID, int floorID, int ceilingID, double yOffset,
		double ySize, double vTop, double vBottom);
	static VoxelDefinition makeDiagonal(int id, bool type1);
	static VoxelDefinition makeTransparentWall(int id, bool collider);
	static VoxelDefinition makeEdge(int id, double yOffset, bool collider, bool flipped, VoxelFacing2D facing);
	static VoxelDefinition makeChasm(int id, ChasmData::Type type);
	static VoxelDefinition makeDoor(int id, DoorData::Type type);

	// Whether this voxel definition contributes to a chasm having a wall face.
	bool allowsChasmFace() const;
};

#endif
