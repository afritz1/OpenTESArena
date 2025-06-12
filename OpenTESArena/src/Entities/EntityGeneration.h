#ifndef ENTITY_GENERATION_H
#define ENTITY_GENERATION_H

#include "EntityDefinition.h"
#include "EntityUtils.h"

class Random;

struct EntityAnimationDefinition;

struct EntityGenInfo
{
	bool nightLightsAreActive;

	// @todo? (might also affect EntityAnimationDef/Inst)
	// - optional rulerIsMale
	// - optional interiorType

	void init(bool nightLightsAreActive);
};

#endif
