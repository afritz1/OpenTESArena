#include "CharacterClassDefinition.h"
#include "CharacterClassLibrary.h"
#include "EntityDefinition.h"
#include "EntityDefinitionLibrary.h"
#include "EntityUtils.h"

std::string EntityUtils::defTypeToString(const EntityDefinition &entityDef)
{
	const EntityDefinition::Type type = entityDef.getType();

	switch (type)
	{
	case EntityDefinition::Type::Citizen:
		return "Citizen";
	case EntityDefinition::Type::Container:
		return "Container";
	case EntityDefinition::Type::Doodad:
		return "Doodad";
	case EntityDefinition::Type::Enemy:
		return "Enemy";
	case EntityDefinition::Type::Item:
		return "Item";
	case EntityDefinition::Type::Projectile:
		return "Projectile";
	case EntityDefinition::Type::StaticNPC:
		return "StaticNPC";
	case EntityDefinition::Type::Transition:
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

int EntityUtils::getYOffset(const EntityDefinition &entityDef)
{
	const EntityDefinition::Type type = entityDef.getType();
	const bool isEnemy = type == EntityDefinition::Type::Enemy;
	const bool isDoodad = type == EntityDefinition::Type::Doodad;
	if (!isEnemy && !isDoodad)
	{
		return 0;
	}

	if (isEnemy)
	{
		const auto &enemyDef = entityDef.getEnemy();
		if (enemyDef.getType() != EntityDefinition::EnemyDefinition::Type::Creature)
		{
			return 0;
		}

		const auto &creatureDef = enemyDef.getCreature();
		return creatureDef.yOffset;
	}
	else
	{
		const auto &doodadDef = entityDef.getDoodad();
		return doodadDef.yOffset;
	}
}

bool EntityUtils::tryGetLightIntensity(const EntityDefinition &entityDef, int *outIntensity)
{
	if (entityDef.getType() != EntityDefinition::Type::Doodad)
	{
		return false;
	}

	const auto &doodadDef = entityDef.getDoodad();
	if (doodadDef.lightIntensity <= 0)
	{
		return false;
	}

	*outIntensity = doodadDef.lightIntensity;
	return true;
}

bool EntityUtils::tryGetDisplayName(const EntityDefinition &entityDef,
	const CharacterClassLibrary &charClassLibrary, std::string *outName)
{
	const EntityDefinition::Type type = entityDef.getType();
	const bool isEnemy = type == EntityDefinition::Type::Enemy;
	if (!isEnemy)
	{
		return false;
	}

	const auto &enemyDef = entityDef.getEnemy();
	const auto enemyType = enemyDef.getType();
	if (enemyType == EntityDefinition::EnemyDefinition::Type::Creature)
	{
		const auto &creatureDef = enemyDef.getCreature();
		*outName = creatureDef.name;
	}
	else if (enemyType == EntityDefinition::EnemyDefinition::Type::Human)
	{
		const auto &humanDef = enemyDef.getHuman();
		const auto &charClass = charClassLibrary.getDefinition(humanDef.charClassID);
		*outName = charClass.getName();
	}
	else
	{
		DebugNotImplementedMsg(std::to_string(static_cast<int>(enemyType)));
	}

	return true;
}
