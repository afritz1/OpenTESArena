#ifndef WORLD_DATA_H
#define WORLD_DATA_H

#include <string>
#include <vector>

#include "VoxelUtils.h"
#include "../Math/Vector2.h"

// Base class for instances of world data (exteriors and interiors).

class INFFile;
class LevelData;

enum class WorldType;

class WorldData
{
protected:
	std::vector<NewDouble2> startPoints;

	WorldData();
public:
	virtual ~WorldData();

	// Gets the start points within each level.
	const std::vector<NewDouble2> &getStartPoints() const;
	
	// Gets the root type of the world (unaffected by the active level).
	virtual WorldType getBaseWorldType() const = 0;

	// Gets the active type of the world (city, interior, wilderness). This is useful
	// since an interior world can be nested inside an exterior world.
	virtual WorldType getActiveWorldType() const = 0;

	// Gets a reference to the active level data (a polymorphic type).
	virtual LevelData &getActiveLevel() = 0;
	virtual const LevelData &getActiveLevel() const = 0;
};

#endif
