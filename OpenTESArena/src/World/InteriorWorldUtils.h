#ifndef INTERIOR_WORLD_UTILS_H
#define INTERIOR_WORLD_UTILS_H

#include <string>

class ArenaRandom;

namespace InteriorWorldUtils
{
	// Default dungeon .MIF with chunks for random generation.
	const std::string DUNGEON_MIF_NAME = "RANDOM1.MIF";

	// Determines how many levels a dungeon has.
	int generateDungeonLevelCount(bool isArtifactDungeon, ArenaRandom &random);
}

#endif
