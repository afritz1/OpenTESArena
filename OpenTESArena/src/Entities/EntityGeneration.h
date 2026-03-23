#ifndef ENTITY_GENERATION_H
#define ENTITY_GENERATION_H

#include "EntityUtils.h"

enum class ArenaCityType;
enum class ArenaInteriorType;

struct EntityGenInfo
{
	bool nightLightsAreActive;

	// For loot generation. Defaults to 0 if undefined for the active scene.
	ArenaCityType cityType;
	ArenaInteriorType interiorType;
	int interiorLevelIndex;

	void init(bool nightLightsAreActive, ArenaCityType cityType, ArenaInteriorType interiorType, int interiorLevelIndex);
};

#endif
