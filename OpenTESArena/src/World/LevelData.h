#ifndef LEVEL_DATA_H
#define LEVEL_DATA_H

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

#include "VoxelGrid.h"
#include "../Assets/MIFFile.h"
#include "../Math/Vector2.h"

// Holds all the data necessary for defining the contents of a level.

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

class INFFile;

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
		~Lock();

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
		~TextTrigger();

		const std::string &getText() const;
		bool isSingleDisplay() const;
		bool hasBeenDisplayed() const;
		void setPreviouslyDisplayed(bool previouslyDisplayed);
	};
private:
	std::unordered_map<Int2, Lock> locks;
	std::unordered_map<Int2, TextTrigger> textTriggers;
	std::unordered_map<Int2, std::string> soundTriggers;
	std::unique_ptr<uint32_t> interiorSkyColor; // Null for exteriors, non-null for interiors.
	VoxelGrid voxelGrid;
	std::string name, infName;
	double ceilingHeight;
	bool outdoorDungeon;

	// Private constructor for static LevelData load methods.
	LevelData(int gridWidth, int gridHeight, int gridDepth);

	void setVoxel(int x, int y, int z, uint8_t id);
	void readFLOR(const std::vector<uint8_t> &flor, const INFFile &inf, int width, int depth);
	void readMAP1(const std::vector<uint8_t> &map1, const INFFile &inf, int width, int depth);
	void readMAP2(const std::vector<uint8_t> &map2, const INFFile &inf, int width, int depth);
	void readCeiling(const INFFile &inf, int width, int depth);
	void readLocks(const std::vector<MIFFile::Level::Lock> &locks, int width, int depth);
	void readTriggers(const std::vector<MIFFile::Level::Trigger> &triggers, const INFFile &inf,
		int width, int depth);
public:
	LevelData(VoxelGrid &&voxelGrid); // Used with test city.
	LevelData(LevelData &&levelData) = default;
	~LevelData();

	// Interior level. The .INF is obtained from the level's info member.
	static LevelData loadInterior(const MIFFile::Level &level, int gridWidth, int gridDepth);

	// Premade exterior level with a pre-defined .INF file. Only used by center province.
	static LevelData loadPremadeCity(const MIFFile::Level &level, const INFFile &inf,
		int gridWidth, int gridDepth);

	// Exterior level with a pre-defined .INF file (for randomly generated cities). This loads
	// the skeleton of the level (city walls, etc.), and fills in the rest by loading the
	// required .MIF chunks.
	static LevelData loadCity(const MIFFile::Level &level, const INFFile &inf,
		int gridWidth, int gridDepth);

	bool isOutdoorDungeon() const;
	double getCeilingHeight() const;

	const std::string &getName() const;
	const std::string &getInfName() const;

	// Gets the level's interior sky color. Causes an error if the level is an exterior.
	// Exteriors have dynamic sky palettes, so it cannot be stored by the level data.
	uint32_t getInteriorSkyColor() const;

	VoxelGrid &getVoxelGrid();
	const VoxelGrid &getVoxelGrid() const;

	// Returns a pointer to some lock if the given voxel has a lock, or null if it doesn't.
	const Lock *getLock(const Int2 &voxel) const;

	// Returns a pointer to some trigger text if the given voxel has a text trigger, or
	// null if it doesn't. Also returns a pointer to one-shot text triggers that have 
	// been activated previously (use another function to check activation).
	TextTrigger *getTextTrigger(const Int2 &voxel);

	// Returns a pointer to a sound filename if the given voxel has a sound trigger, or
	// null if it doesn't.
	const std::string *getSoundTrigger(const Int2 &voxel) const;
};

#endif
