#ifndef LEVEL_UTILS_H
#define LEVEL_UTILS_H

#include "Coord.h"

class LevelDefinition;
class LevelInfoDefinition;
class TransitionDefinition;

namespace LevelUtils
{
	// Tries to get the first transition at the given level coordinate. This is only needed since chunks don't
	// store transition data yet.
	const TransitionDefinition *tryGetTransition(const LevelInt3 &voxel, const LevelDefinition &levelDef,
		const LevelInfoDefinition &levelInfoDef);
}

#endif
