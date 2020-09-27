#ifndef INTERIOR_WORLD_UTILS_H
#define INTERIOR_WORLD_UTILS_H

class ArenaRandom;

namespace InteriorWorldUtils
{
	// Determines how many levels a dungeon has.
	int generateDungeonLevelCount(bool isArtifactDungeon, ArenaRandom &random);
}

#endif
