#ifndef LEVEL_DATA_H
#define LEVEL_DATA_H

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "VoxelGrid.h"
#include "../Assets/ArenaTypes.h"
#include "../Assets/INFFile.h"
#include "../Assets/MIFFile.h"
#include "../Math/Vector2.h"

// Base class for each active "space" in the game. Exteriors only have one level, but
// interiors can have several.

// Arena's level origins start at the top-right corner of the map, so X increases 
// going to the left, and Z increases going down. The wilderness uses this same 
// pattern. Each chunk looks like this:
// +++++++ <- Origin (0, 0)
// +++++++
// +++++++
// +++++++
// ^
// |
// Max (mapWidth - 1, mapDepth - 1)

// Hash specialization for readFLOR() chasm mappings.
namespace std
{
	template <>
	struct hash<std::pair<uint16_t, std::array<bool, 4>>>
	{
		size_t operator()(const std::pair<uint16_t, std::array<bool, 4>> &p) const
		{
			// XOR with some arbitrary prime numbers (not sure if this is any good).
			return static_cast<size_t>(p.first ^
				(p.second.at(0) ? 41 : 73) ^
				(p.second.at(1) ? 89 : 113) ^
				(p.second.at(2) ? 127 : 149) ^
				(p.second.at(3) ? 157 : 193));
		}
	};
}

class ArenaRandom;
class ExeData;
class Renderer;
class TextureManager;

enum class WorldType;

class LevelData
{
public:
	class Lock
	{
	private:
		Int2 position;
		int lockLevel;
	public:
		Lock(const Int2 &position, int lockLevel);

		const Int2 &getPosition() const;
		int getLockLevel() const;
	};

	// Each text trigger is paired with a boolean telling whether it should be displayed once.
	class TextTrigger
	{
	private:
		std::string text;
		bool displayedOnce, previouslyDisplayed;
	public:
		TextTrigger(const std::string &text, bool displayedOnce);

		const std::string &getText() const;
		bool isSingleDisplay() const;
		bool hasBeenDisplayed() const;
		void setPreviouslyDisplayed(bool previouslyDisplayed);
	};

	class DoorState
	{
	public:
		enum class Direction { None, Opening, Closing };
	private:
		static constexpr double DEFAULT_SPEED = 1.30; // @todo: currently arbitrary value.

		Int2 voxel;
		double percentOpen;
		Direction direction;
	public:
		DoorState(const Int2 &voxel, double percentOpen, DoorState::Direction direction);

		// Defaults to opening state (as if the player had just activated it).
		DoorState(const Int2 &voxel);

		const Int2 &getVoxel() const;
		double getPercentOpen() const;

		// Returns whether the door's current direction is closing. This is used to make
		// sure that sounds are only played once when a door begins closing.
		bool isClosing() const;

		// Removed from open doors list when true. The code that manages open doors should
		// update the doors before removing closed ones.
		bool isClosed() const;

		void setDirection(DoorState::Direction direction);
		void update(double dt);
	};
private:
	std::unordered_map<Int2, Lock> locks;

	// Mappings of IDs to voxel data indices. Chasms are treated separately since their voxel
	// data index is also a function of the four adjacent voxels. These maps are stored here
	// because they might be shared between multiple calls to read{FLOR,MAP1,MAP2}().
	std::unordered_map<uint16_t, int> wallDataMappings, floorDataMappings, map2DataMappings;
	std::unordered_map<std::pair<uint16_t, std::array<bool, 4>>, int> chasmDataMappings;

	VoxelGrid voxelGrid;
	INFFile inf;
	std::vector<DoorState> openDoors;
	std::string name;
protected:
	// Used by derived LevelData load methods.
	LevelData(int gridWidth, int gridHeight, int gridDepth, const std::string &infName,
		const std::string &name);

	void setVoxel(int x, int y, int z, uint16_t id);
	void readFLOR(const uint16_t *flor, const INFFile &inf, int gridWidth, int gridDepth);
	void readMAP1(const uint16_t *map1, const INFFile &inf, WorldType worldType,
		int gridWidth, int gridDepth, const ExeData &exeData);
	void readMAP2(const uint16_t *map2, const INFFile &inf, int gridWidth, int gridDepth);
	void readCeiling(const INFFile &inf, int width, int depth);
	void readLocks(const std::vector<ArenaTypes::MIFLock> &locks, int width, int depth);
public:
	LevelData(LevelData&&) = default;
	virtual ~LevelData();

	const std::string &getName() const;
	double getCeilingHeight() const;
	std::vector<DoorState> &getOpenDoors();
	const std::vector<DoorState> &getOpenDoors() const;
	const INFFile &getInfFile() const;
	VoxelGrid &getVoxelGrid();
	const VoxelGrid &getVoxelGrid() const;

	// Returns a pointer to some lock if the given voxel has a lock, or null if it doesn't.
	const Lock *getLock(const Int2 &voxel) const;

	// Returns whether a level is considered an outdoor dungeon. Only true for some interiors.
	virtual bool isOutdoorDungeon() const = 0;

	// Sets this level active in the renderer. It's virtual so derived level data classes can
	// do some extra work (like set interior sky colors in the renderer).
	virtual void setActive(TextureManager &textureManager, Renderer &renderer);

	// Ticks the level data by delta time. Does nothing by default.
	virtual void tick(double dt);
};

#endif
