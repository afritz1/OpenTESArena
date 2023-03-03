#ifndef ENTITY_GENERATION_H
#define ENTITY_GENERATION_H

#include "EntityDefinition.h"
#include "EntityUtils.h"

class Random;

struct EntityAnimationDefinition;

namespace EntityGeneration
{
	struct EntityGenInfo
	{
		bool nightLightsAreActive;

		// @todo? (might also affect EntityAnimationDef/Inst)
		// - optional rulerIsMale
		// - optional interiorType

		void init(bool nightLightsAreActive);
	};

	const std::string &getDefaultAnimationStateName(const EntityDefinition &entityDef, const EntityGenInfo &genInfo);
}

#endif
