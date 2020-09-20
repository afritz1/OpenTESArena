#ifndef VOXEL_DEFINITION_H
#define VOXEL_DEFINITION_H

#include "../Math/Vector3.h"

// The definition of a voxel that a voxel ID points to. Since there will only be a few kinds
// of voxel data per world, their size can be much larger than just a byte or two.

// A voxel's definition is used for multiple things, such as rendering, collision detection,
// and color-coding on the automap.

enum class VoxelDataType;
enum class VoxelFacing;

class VoxelDefinition
{
public:
	// IDs range from 0 to 63.
	static const int TOTAL_IDS;

	// Regular wall with Y size equal to ceiling height. Y offset is 0, and Y size
	// can be inferred by the renderer.
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

		// Not present in original game; necessary for all texture coordinates to be correct,
		// i.e., both palace graphics and store signs.
		bool flipped;

		VoxelFacing facing;
	};

	// Chasms have zero to four wall faces (stored with voxel instance) depending on adjacent
	// floors. Each face is front-facing and back-facing.
	struct ChasmData
	{
		enum class Type { Dry, Wet, Lava };

		// The sizes of wet chasms and lava chasms are unaffected by ceiling height.
		static const double WET_LAVA_DEPTH;

		int id;
		Type type;

		bool matches(const ChasmData &other) const;
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

	VoxelDefinition();

	static VoxelDefinition makeWall(int sideID, int floorID, int ceilingID, const int *menuID,
		WallData::Type type);
	static VoxelDefinition makeFloor(int id);
	static VoxelDefinition makeCeiling(int id);
	static VoxelDefinition makeRaised(int sideID, int floorID, int ceilingID, double yOffset,
		double ySize, double vTop, double vBottom);
	static VoxelDefinition makeDiagonal(int id, bool type1);
	static VoxelDefinition makeTransparentWall(int id, bool collider);
	static VoxelDefinition makeEdge(int id, double yOffset, bool collider, bool flipped, VoxelFacing facing);
	static VoxelDefinition makeChasm(int id, ChasmData::Type type);
	static VoxelDefinition makeDoor(int id, DoorData::Type type);

	// Gets the normal associated with a voxel facing.
	static Double3 getNormal(VoxelFacing facing);
};

#endif
