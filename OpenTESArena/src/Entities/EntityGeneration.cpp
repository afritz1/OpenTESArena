#include "EntityGeneration.h"
#include "EntityManager.h"
#include "EntityType.h"
#include "../Game/CardinalDirection.h"

Entity *EntityGeneration::makeEntity(EntityType entityType, EntityDefinition::Type entityDefType,
	EntityDefID entityDefID, const EntityAnimationDefinition &animDef, Random &random, EntityManager &entityManager)
{
	EntityRef entity = entityManager.makeEntity(entityType); // @todo: decide if chunk should be an argument too
	Entity *entityPtr = entity.get();

	EntityAnimationInstance animInst;
	animInst.init(animDef);

	const std::optional<int> defaultStateIndex = animDef.tryGetStateIndex(
		EntityAnimationUtils::STATE_IDLE.c_str());
	if (!defaultStateIndex.has_value())
	{
		DebugLogWarning("Couldn't get default state index for entity.");
		return nullptr;
	}

	animInst.setStateIndex(*defaultStateIndex);
	// @todo: set anim inst to the correct state for the entity
	// @todo: let the entity acquire the anim inst instead of copying
	// @todo: set streetlight anim state if night lights are active

	if (entityType == EntityType::Static)
	{
		StaticEntity *staticEntity = dynamic_cast<StaticEntity*>(entityPtr);
		if (entityDefType == EntityDefinition::Type::StaticNPC)
		{
			staticEntity->initNPC(entityDefID, animInst);
		}
		else if (entityDefType == EntityDefinition::Type::Item)
		{
			// @todo: initialize as an item
			staticEntity->initDoodad(entityDefID, animInst);
			DebugLogError("Item entity initialization not implemented.");
		}
		else if (entityDefType == EntityDefinition::Type::Container)
		{
			staticEntity->initContainer(entityDefID, animInst);
		}
		else if (entityDefType == EntityDefinition::Type::Transition)
		{
			staticEntity->initTransition(entityDefID, animInst);
		}
		else if (entityDefType == EntityDefinition::Type::Doodad)
		{
			staticEntity->initDoodad(entityDefID, animInst);
		}
		else
		{
			DebugNotImplementedMsg(std::to_string(static_cast<int>(entityDefType)));
		}
	}
	else if (entityType == EntityType::Dynamic)
	{
		DynamicEntity *dynamicEntity = dynamic_cast<DynamicEntity*>(entityPtr);
		const VoxelDouble2 direction = CardinalDirection::North;
		const CardinalDirectionName cardinalDirection = CardinalDirection::getDirectionName(direction);

		if (entityDefType == EntityDefinition::Type::Enemy)
		{
			dynamicEntity->initCreature(entityDefID, animInst, direction, random);
		}
		else
		{
			DebugNotImplementedMsg(std::to_string(static_cast<int>(entityDefType)));
		}
	}
	else
	{
		DebugNotImplementedMsg(std::to_string(static_cast<int>(entityType)));
	}

	return entityPtr;
}
