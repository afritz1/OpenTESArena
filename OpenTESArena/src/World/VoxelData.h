#ifndef VOXEL_DATA_H
#define VOXEL_DATA_H

#include "../Math/Vector3.h"

// Voxel data is the definition of a voxel that a voxel ID points to. Since there will 
// only be a few kinds of voxel data per world, their size can be much larger than just 
// a byte or two.

// A voxel's data is used for multiple things, such as rendering, collision detection,
// and color-coding on the automap.

enum class VoxelDataType;

class VoxelData
{
public:
	// IDs range from 0 to 63.
	static const int TOTAL_IDS;

	// This defines which axis a wall's normal is facing towards on the outside
	// (i.e., away from the center of the voxel). Used with edges and rendering.
	// The Y axis is elided for simplicity (although it might be added eventually).
	enum class Facing { PositiveX, NegativeX, PositiveZ, NegativeZ };

	// Regular wall with Y size equal to ceiling height. Y offset is 0, and Y size
	// can be inferred by the renderer on the main floor.
	struct WallData
	{
		enum class Type { Solid, LevelUp, LevelDown, Menu };

		// Maps one or more *MENU IDs to a type of menu voxel, for city and wilderness menus.
		// Cities and the wilderness interpret the ID differently.
		enum class MenuType
		{
			None,
			CityGates,
			Crypt, // WCRYPT
			Dungeon, // DUNGEON
			Equipment, // EQUIP
			House, // BS
			MagesGuild, // MAGE
			Noble, // NOBLE
			Palace, // PALACE
			Tavern, // TAVERN
			Temple, // TEMPLE
			Tower // TOWER
		};

		int sideID, floorID, ceilingID, menuID;
		Type type;

		// Returns whether the wall data is for a *MENU block.
		bool isMenu() const;

		// Gets exterior menu type from *MENU ID and city boolean, or "none" if no mapping exists.
		static MenuType getMenuType(int menuID, bool isCity);

		// Returns whether the menu type is for an interior (equipment, tavern, etc.) or something
		// else (like city gates).
		static bool menuLeadsToInterior(MenuType menuType);

		// Returns whether the menu type displays text on-screen when the player right clicks it.
		static bool menuHasDisplayName(MenuType menuType);
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

		// The sizes of wet chasms and lava chasms are unaffected by ceiling height.
		static const double WET_LAVA_DEPTH;

		int id;
		bool north, east, south, west;
		Type type;

		bool faceIsVisible(VoxelData::Facing facing) const;
	};

	struct DoorData
	{
		// Each type of door. Most doors swing open, while others raise up or slide to the side.
		// Splitting doors are unused in the original game.
		enum class Type { Swinging, Sliding, Raising, Splitting };

		// Each door has a certain behavior for playing sounds when closing.
		enum class CloseSoundType { OnClosed, OnClosing };

		struct CloseSoundData
		{
			int soundIndex;
			CloseSoundType type;
		};

		int id;
		Type type;

		// Gets the door's open sound index.
		int getOpenSoundIndex() const;

		// Gets the door's close sound data.
		CloseSoundData getCloseSoundData() const;
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

	// Gets the normal associated with a voxel facing.
	static Double3 getNormal(VoxelData::Facing facing);
};

#endif
