#ifndef VOXEL_DATA_H
#define VOXEL_DATA_H

// Voxel data is the definition of a voxel that a voxel ID points to. Since there will 
// only be a few kinds of voxel data per world, their size can be much larger than just 
// a byte or two.

// A voxel's data is used for multiple things, such as rendering, collision detection,
// and color-coding on the automap.

// Perhaps, when voxel destruction spells like Passwall are added, more data could be 
// added here that represents a percentage of "fade".

enum class VoxelDataType;

class VoxelData
{
public:
	// IDs range from 0 to 63.
	static const int TOTAL_IDS;

	// This defines which axis a wall's normal is facing towards on the outside
	// (i.e., away from the center of the voxel). Used with edges and rendering.
	enum class Facing { PositiveX, NegativeX, PositiveZ, NegativeZ };

	// Regular wall with Y size equal to ceiling height if on main floor, otherwise 1.0.
	// Y offset is 0, and Y size can be inferred by the renderer on the main floor.
	struct WallData
	{
		enum class Type { Solid, LevelUp, LevelDown, Menu };

		int sideID, floorID, ceilingID, menuID;
		Type type;
	};

	// Floors only have their top rendered.
	struct FloorData
	{
		int id;
	};

	// Ceilings only have their bottom rendered.
	struct CeilingData
	{
		int id;
	};

	// Raised platform.
	struct RaisedData
	{
		int sideID, floorID, ceilingID;
		double yOffset, ySize, vTop, vBottom;
	};

	// Diagonal. The type determines the start and end corners.
	struct DiagonalData
	{
		int id;
		bool type1; // Type 1 is '/', (nearX, nearZ) -> (farX, farZ).
	};

	// Transparent walls only shows front-facing textures (wooden arches, hedges, etc.).
	// Nothing is drawn when the player is in the same voxel column.
	struct TransparentWallData
	{
		int id;
		bool collider; // Also affects automap visibility.
	};

	// Rendered on one edge of a voxel with height equal to ceiling height.
	// The facing determines which side the edge is on.
	struct EdgeData
	{
		int id;
		double yOffset;
		bool collider;
		Facing facing;
	};

	// Chasms have zero to four visible faces depending on adjacent floors. Each face is 
	// front-facing and back-facing.
	struct ChasmData
	{
		enum class Type { Dry, Wet, Lava };

		int id;
		bool north, east, south, west;
		Type type;

		bool faceIsVisible(VoxelData::Facing facing) const;
	};

	struct DoorData
	{
		// Each type of door. Most doors swing open, while others raise up or slide to the side.
		// Splitting doors are unused.
		enum class Type { Swinging, Sliding, Raising };

		int id;
		Type type;
	};

	VoxelDataType dataType; // Defines how the voxel is interpreted and rendered.

	// Only one voxel data type can be active at a time, given by "dataType".
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

	VoxelData();

	static VoxelData makeWall(int sideID, int floorID, int ceilingID, const int *menuID,
		WallData::Type type);
	static VoxelData makeFloor(int id);
	static VoxelData makeCeiling(int id);
	static VoxelData makeRaised(int sideID, int floorID, int ceilingID, double yOffset,
		double ySize, double vTop, double vBottom);
	static VoxelData makeDiagonal(int id, bool type1);
	static VoxelData makeTransparentWall(int id, bool collider);
	static VoxelData makeEdge(int id, double yOffset, bool collider, Facing facing);
	static VoxelData makeChasm(int id, bool north, bool east, bool south, bool west,
		ChasmData::Type type);
	static VoxelData makeDoor(int id, DoorData::Type type);
};

#endif
