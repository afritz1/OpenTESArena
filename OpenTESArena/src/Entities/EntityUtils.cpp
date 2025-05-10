#include <algorithm>

#include "EntityDefinition.h"
#include "EntityDefinitionLibrary.h"
#include "EntityUtils.h"
#include "../Math/Random.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Stats/CharacterClassDefinition.h"
#include "../Stats/CharacterClassLibrary.h"
#include "../World/ChunkUtils.h"

#include "components/debug/Debug.h"

bool EntityUtils::isDynamicEntity(EntityDefinitionType defType)
{
	switch (defType)
	{
	case EntityDefinitionType::StaticNPC:
	case EntityDefinitionType::Item:
	case EntityDefinitionType::Container:
	case EntityDefinitionType::Transition:
	case EntityDefinitionType::Decoration:
		return false;
	case EntityDefinitionType::Enemy:
	case EntityDefinitionType::Citizen:
	case EntityDefinitionType::Projectile:
		return true;
	default:
		DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(defType)));
	}
}

std::string EntityUtils::defTypeToString(const EntityDefinition &entityDef)
{
	const EntityDefinitionType type = entityDef.type;

	switch (type)
	{
	case EntityDefinitionType::Citizen:
		return "Citizen";
	case EntityDefinitionType::Container:		
	{
		const ContainerEntityDefinitionType containerType = entityDef.container.type;
		switch (containerType)
		{
		case ContainerEntityDefinitionType::Holder:
			return std::string("Holder") + (entityDef.container.holder.locked ? " Locked" : " Unlocked");
		case ContainerEntityDefinitionType::Pile:
			return "Pile";
		default:
			DebugUnhandledReturnMsg(std::string, std::to_string(static_cast<int>(containerType)));
		}
	}
	case EntityDefinitionType::Decoration:
		return "Decoration";
	case EntityDefinitionType::Enemy:
		return "Enemy";
	case EntityDefinitionType::Item:
	{
		const ItemEntityDefinitionType itemType = entityDef.item.type;
		switch (itemType)
		{
		case ItemEntityDefinitionType::Key:
			return "Key";
		case ItemEntityDefinitionType::QuestItem:
			return "Quest Item";
		default:
			DebugUnhandledReturnMsg(std::string, std::to_string(static_cast<int>(itemType)));
		}
	}
	case EntityDefinitionType::Projectile:
		return "Projectile";
	case EntityDefinitionType::StaticNPC:
		return "StaticNPC";
	case EntityDefinitionType::Transition:
		return "Transition";
	default:
		DebugUnhandledReturnMsg(std::string, std::to_string(static_cast<int>(type)));
	}
}

bool EntityUtils::isLevelDependentDef(EntityDefID defID,
	const EntityDefinitionLibrary &entityDefLibrary)
{
	return (defID >= 0) && (defID < entityDefLibrary.getDefinitionCount());
}

bool EntityUtils::isStreetlight(const EntityDefinition &entityDef)
{
	return (entityDef.type == EntityDefinitionType::Decoration) && entityDef.decoration.streetlight;
}

bool EntityUtils::isGhost(const EntityDefinition &entityDef)
{
	if (entityDef.type != EntityDefinitionType::Enemy)
	{
		return false;
	}

	const EnemyEntityDefinition &enemyDef = entityDef.enemy;
	if (enemyDef.type != EnemyEntityDefinitionType::Creature)
	{
		return false;
	}

	return enemyDef.creature.ghost;
}

bool EntityUtils::isPuddle(const EntityDefinition &entityDef)
{
	if (entityDef.type != EntityDefinitionType::Decoration)
	{
		return false;
	}

	const DecorationEntityDefinition &decoration = entityDef.decoration;
	return decoration.puddle;
}

int EntityUtils::getYOffset(const EntityDefinition &entityDef)
{
	const EntityDefinitionType type = entityDef.type;
	const bool isEnemy = type == EntityDefinitionType::Enemy;
	const bool isItem = type == EntityDefinitionType::Item;
	const bool isDecoration = type == EntityDefinitionType::Decoration;
	if (!isEnemy && !isItem && !isDecoration)
	{
		return 0;
	}

	if (isEnemy)
	{
		const EnemyEntityDefinition &enemyDef = entityDef.enemy;
		if (enemyDef.type != EnemyEntityDefinitionType::Creature)
		{
			return 0;
		}

		const EnemyEntityDefinition::CreatureDefinition &creatureDef = enemyDef.creature;
		return creatureDef.yOffset;
	}
	else if (isItem)
	{
		const ItemEntityDefinition &itemDef = entityDef.item;
		if (itemDef.type == ItemEntityDefinitionType::Key)
		{
			return 0;
		}

		const ItemEntityDefinition::QuestItemDefinition &questItemDef = itemDef.questItem;
		return questItemDef.yOffset;
	}
	else
	{
		const DecorationEntityDefinition &decorationDef = entityDef.decoration;
		return decorationDef.yOffset;
	}
}

bool EntityUtils::hasCollision(const EntityDefinition &entityDef)
{
	const EntityDefinitionType entityType = entityDef.type;
	switch (entityType)
	{
	case EntityDefinitionType::Enemy:
	case EntityDefinitionType::StaticNPC:
		return true;
	case EntityDefinitionType::Citizen:
	case EntityDefinitionType::Container:
	case EntityDefinitionType::Item:
	case EntityDefinitionType::Projectile:
	case EntityDefinitionType::Transition:
		return false;
	case EntityDefinitionType::Decoration:
		return entityDef.decoration.collider;
	default:
		DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(entityType)));
	}
}

bool EntityUtils::canDie(const EntityDefinition &entityDef)
{
	const EntityDefinitionType entityType = entityDef.type;
	switch (entityType)
	{
	case EntityDefinitionType::Enemy:
	case EntityDefinitionType::Citizen:
		return true;
	case EntityDefinitionType::StaticNPC:
	case EntityDefinitionType::Item:
	case EntityDefinitionType::Container:
	case EntityDefinitionType::Projectile:
	case EntityDefinitionType::Transition:
	case EntityDefinitionType::Decoration:
		return false;
	default:
		DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(entityType)));
	}
}

std::optional<int> EntityUtils::tryGetDeathAnimStateIndex(const EntityAnimationDefinition &animDef)
{
	const std::optional<int> deathStateIndex = animDef.tryGetStateIndex(EntityAnimationUtils::STATE_DEATH.c_str());
	return deathStateIndex;
}

bool EntityUtils::leavesCorpse(const EntityDefinition &entityDef)
{
	const EntityDefinitionType entityType = entityDef.type;
	switch (entityType)
	{
	case EntityDefinitionType::Enemy:
	{
		const EnemyEntityDefinition &enemyDef = entityDef.enemy;
		const EnemyEntityDefinitionType enemyDefType = enemyDef.type;
		if (enemyDefType == EnemyEntityDefinitionType::Human)
		{
			return true;
		}
		else if (enemyDefType == EnemyEntityDefinitionType::Creature)
		{
			return !enemyDef.creature.hasNoCorpse;
		}
		else
		{
			DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(enemyDefType)));
		}
	}		
	case EntityDefinitionType::Citizen:
	case EntityDefinitionType::StaticNPC:
	case EntityDefinitionType::Item:
	case EntityDefinitionType::Container:
	case EntityDefinitionType::Projectile:
	case EntityDefinitionType::Transition:
	case EntityDefinitionType::Decoration:
		return false;
	default:
		DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(entityType)));
	}
}

std::optional<double> EntityUtils::tryGetLightRadius(const EntityDefinition &entityDef)
{
	constexpr double lightUnitsRatio = MIFUtils::ARENA_UNITS / 100.0;

	const EntityDefinitionType entityDefType = entityDef.type;
	switch (entityDefType)
	{
	case EntityDefinitionType::Item:
	{
		const ItemEntityDefinition &itemDef = entityDef.item;
		if (itemDef.type == ItemEntityDefinitionType::QuestItem)
		{
			const ItemEntityDefinition::QuestItemDefinition &questItemDef = itemDef.questItem;
			const bool isStaffPiece = questItemDef.yOffset != 0; // @todo: maybe ask "is main quest item"?
			if (isStaffPiece)
			{
				return 2.0 * lightUnitsRatio; // @todo this should take the .INF value properly
			}			
		}

		break;
	}		
	case EntityDefinitionType::Decoration:
	{
		const DecorationEntityDefinition &decorationDef = entityDef.decoration;
		if (decorationDef.streetlight)
		{
			return ArenaRenderUtils::STREETLIGHT_LIGHT_RADIUS;
		}
		else if (decorationDef.lightIntensity > 0)
		{
			return static_cast<double>(decorationDef.lightIntensity) * lightUnitsRatio;
		}

		break;
	}
	default:
		return std::nullopt;
	}

	return std::nullopt;
}

void EntityUtils::getAnimationMaxDims(const EntityAnimationDefinition &animDef, double *outMaxWidth, double *outMaxHeight)
{
	double maxAnimWidth = 0.0;
	double maxAnimHeight = 0.0;

	for (int i = 0; i < animDef.keyframeCount; i++)
	{
		const EntityAnimationDefinitionKeyframe &keyframe = animDef.keyframes[i];
		maxAnimWidth = std::max(maxAnimWidth, keyframe.width);
		maxAnimHeight = std::max(maxAnimHeight, keyframe.height);
	}

	*outMaxWidth = maxAnimWidth;
	*outMaxHeight = maxAnimHeight;
}

bool EntityUtils::tryGetDisplayName(const EntityDefinition &entityDef, const CharacterClassLibrary &charClassLibrary, std::string *outName)
{
	const EntityDefinitionType type = entityDef.type;
	const bool isEnemy = type == EntityDefinitionType::Enemy;
	if (!isEnemy)
	{
		return false;
	}

	const EnemyEntityDefinition &enemyDef = entityDef.enemy;
	const EnemyEntityDefinitionType enemyType = enemyDef.type;
	if (enemyType == EnemyEntityDefinitionType::Creature)
	{
		const auto &creatureDef = enemyDef.creature;
		*outName = creatureDef.name;
	}
	else if (enemyType == EnemyEntityDefinitionType::Human)
	{
		const auto &humanDef = enemyDef.human;
		const auto &charClass = charClassLibrary.getDefinition(humanDef.charClassID);
		*outName = charClass.name;
	}
	else
	{
		DebugNotImplementedMsg(std::to_string(static_cast<int>(enemyType)));
	}

	return true;
}

bool EntityUtils::withinHearingDistance(const WorldDouble3 &listenerPosition, const WorldDouble3 &soundPosition)
{
	const Double3 diff = soundPosition - listenerPosition;
	constexpr double hearingDistanceSqr = EntityUtils::HearingDistance * EntityUtils::HearingDistance;
	return diff.lengthSquared() < hearingDistanceSqr;
}

double EntityUtils::nextCreatureSoundWaitSeconds(Random &random)
{
	// Arbitrary amount of time.
	return 2.75 + (random.nextReal() * 4.50);
}
