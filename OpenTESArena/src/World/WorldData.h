#ifndef WORLD_DATA_H
#define WORLD_DATA_H

#include <string>
#include <unordered_map>

#include "VoxelGrid.h"
#include "../Entities/EntityManager.h"
#include "../Math/Vector2.h"

// This class stores data regarding elements in the game world. It should be constructible
// from a pair of .MIF and .INF files.

class INFFile;
class MIFFile;

class WorldData
{
public:
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
	std::unordered_map<Int2, TextTrigger> textTriggers;
	std::unordered_map<Int2, std::string> soundTriggers;
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
	TextTrigger *getTextTrigger(const Int2 &voxel);

	// Returns a pointer to a sound filename if the given voxel has a sound trigger, or
	// null if it doesn't.
	const std::string *getSoundTrigger(const Int2 &voxel) const;
};

#endif
