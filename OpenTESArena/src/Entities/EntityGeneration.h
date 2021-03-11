#ifndef ENTITY_GENERATION_H
#define ENTITY_GENERATION_H

#include "EntityDefinition.h"
#include "EntityUtils.h"

class Entity;
class EntityAnimationDefinition;
class EntityManager;
class Random;

enum class EntityType;

namespace EntityGeneration
{
	/*EntityGenerationInfo (might also affect EntityAnimationDef / Inst)
	- optional rulerIsMale
	- optional interiorType*/

	Entity *makeEntity(EntityType entityType, EntityDefinition::Type entityDefType, EntityDefID entityDefID,
		const EntityAnimationDefinition &animDef, Random &random, EntityManager &entityManager);
}

#endif
