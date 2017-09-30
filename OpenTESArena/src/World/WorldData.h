#ifndef WORLD_DATA_H
#define WORLD_DATA_H

#include <string>
#include <unordered_map>
#include <unordered_set>

#include "VoxelGrid.h"
#include "../Entities/EntityManager.h"
#include "../Math/Vector2.h"

// This class stores data regarding elements in the game world. It should be constructible
// from a pair of .MIF and .INF files.

class INFFile;
class MIFFile;

class WorldData
{
private:
	// Each text trigger is paired with a bool telling whether it should be displayed once.
	std::unordered_map<Int2, std::pair<bool, std::string>> textTriggers;
	std::unordered_map<Int2, std::string> soundTriggers;
	std::unordered_set<Int2> activatedOneShotTextTriggers;
	VoxelGrid voxelGrid;
	EntityManager entityManager;
public:
	WorldData(const MIFFile &mif, const INFFile &inf);
	WorldData(VoxelGrid &&voxelGrid, EntityManager &&entityManager);
	WorldData(WorldData &&worldData) = default;
	~WorldData();

	WorldData &operator=(WorldData &&worldData) = default;

	VoxelGrid &getVoxelGrid();
	const VoxelGrid &getVoxelGrid() const;

	EntityManager &getEntityManager();
	const EntityManager &getEntityManager() const;

	// Returns a pointer to some trigger text if the given voxel has a text trigger, or
	// null if it doesn't. Also returns a pointer to one-shot text triggers that have 
	// been activated previously (use another function to check activation).
	const std::string *getTextTrigger(const Int2 &voxel) const;

	// Returns true if the text trigger in the given voxel should only be displayed once.
	bool textTriggerIsSingleDisplay(const Int2 &voxel) const;

	// Returns true if the text trigger in the given voxel is a one-shot text trigger that 
	// has already been displayed, or false if it doesn't exist or hasn't been played.
	bool singleDisplayTextTriggerActivated(const Int2 &voxel) const;

	// Sets a one-shot text trigger as activated.
	void activateSingleDisplayTextTrigger(const Int2 &voxel);

	// Returns a pointer to a sound filename if the given voxel has a sound trigger, or
	// null if it doesn't.
	const std::string *getSoundTrigger(const Int2 &voxel) const;
};

#endif
