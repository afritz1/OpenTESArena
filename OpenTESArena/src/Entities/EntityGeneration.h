#ifndef ENTITY_GENERATION_H
#define ENTITY_GENERATION_H

#include "EntityDefinition.h"
#include "EntityUtils.h"

class Entity;
class EntityManager;
class Random;

struct EntityAnimationDefinition;

enum class EntityType;

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

	Entity *makeEntity(EntityType entityType, EntityDefinition::Type entityDefType, EntityDefID entityDefID,
		const EntityDefinition &entityDef, const EntityAnimationDefinition &animDef,
		const EntityGenInfo &entityGenInfo, Random &random, EntityManager &entityManager);
}

#endif
