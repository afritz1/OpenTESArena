#ifndef LEVEL_DATA_H
#define LEVEL_DATA_H

#include <string>
#include <unordered_map>

#include "VoxelGrid.h"
#include "../Assets/MIFFile.h"
#include "../Math/Vector2.h"

// Holds all the data necessary for defining the contents of a level.

class INFFile;

class LevelData
{
public:
	enum class FloorType { None, Solid, Dry, Wet, Lava };
	enum class WallType { None, Solid, Raised, Diag1, Diag2, Door, Transparent };

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
	VoxelGrid voxelGrid;
public:
	LevelData(const MIFFile::Level &level, const INFFile &inf, int gridWidth, 
		int gridDepth, bool isInterior);
	LevelData(VoxelGrid &&voxelGrid); // Used with test city.
	LevelData(LevelData &&levelData) = default;
	~LevelData();

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
