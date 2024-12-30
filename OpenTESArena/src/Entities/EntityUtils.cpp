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
	case EntityDefinitionType::Doodad:
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
	const EntityDefinitionType type = entityDef.getType();

	switch (type)
	{
	case EntityDefinitionType::Citizen:
		return "Citizen";
	case EntityDefinitionType::Container:
		return "Container";
	case EntityDefinitionType::Doodad:
		return "Doodad";
	case EntityDefinitionType::Enemy:
		return "Enemy";
	case EntityDefinitionType::Item:
		return "Item";
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
	return (entityDef.getType() == EntityDefinitionType::Doodad) && entityDef.getDoodad().streetlight;
}

bool EntityUtils::isGhost(const EntityDefinition &entityDef)
{
	if (entityDef.getType() != EntityDefinitionType::Enemy)
	{
		return false;
	}

	const EntityDefinition::EnemyDefinition &enemyDef = entityDef.getEnemy();
	if (enemyDef.getType() != EntityDefinition::EnemyDefinition::Type::Creature)
	{
		return false;
	}

	return enemyDef.getCreature().ghost;
}

bool EntityUtils::isPuddle(const EntityDefinition &entityDef)
{
	if (entityDef.getType() != EntityDefinitionType::Doodad)
	{
		return false;
	}

	const EntityDefinition::DoodadDefinition &doodad = entityDef.getDoodad();
	return doodad.puddle;
}

int EntityUtils::getYOffset(const EntityDefinition &entityDef)
{
	const EntityDefinitionType type = entityDef.getType();
	const bool isEnemy = type == EntityDefinitionType::Enemy;
	const bool isDoodad = type == EntityDefinitionType::Doodad;
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

bool EntityUtils::hasCollision(const EntityDefinition &entityDef)
{
	const EntityDefinitionType entityType = entityDef.getType();
	switch (entityType)
	{
	case EntityDefinitionType::Enemy:
	case EntityDefinitionType::StaticNPC:
	case EntityDefinitionType::Container:
		return true;
	case EntityDefinitionType::Citizen:
	case EntityDefinitionType::Item:
	case EntityDefinitionType::Projectile:
	case EntityDefinitionType::Transition:
		return false;
	case EntityDefinitionType::Doodad:
		return entityDef.getDoodad().collider;
	default:
		DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(entityType)));
	}
}

std::optional<double> EntityUtils::tryGetLightRadius(const EntityDefinition &entityDef)
{
	if (entityDef.getType() != EntityDefinitionType::Doodad)
	{
		return std::nullopt;
	}

	const EntityDefinition::DoodadDefinition &doodadDef = entityDef.getDoodad();
	if (doodadDef.streetlight)
	{
		return ArenaRenderUtils::STREETLIGHT_LIGHT_RADIUS;
	}
	else if (doodadDef.lightIntensity > 0)
	{
		return static_cast<double>(doodadDef.lightIntensity);
	}
	else
	{
		return std::nullopt;
	}
}

void EntityUtils::getAnimationMaxDims(const EntityAnimationDefinition &animDef, double *outMaxWidth, double *outMaxHeight)
{
	double maxAnimWidth = 0.0;
	double maxAnimHeight = 0.0;
	for (int i = 0; i < animDef.stateCount; i++)
	{
		const EntityAnimationDefinitionState &state = animDef.states[i];
		for (int j = 0; j < state.keyframeListCount; j++)
		{
			const int keyframeListIndex = state.keyframeListsIndex + j;
			const EntityAnimationDefinitionKeyframeList &keyframeList = animDef.keyframeLists[keyframeListIndex];
			for (int k = 0; k < keyframeList.keyframeCount; k++)
			{
				const int keyframeIndex = keyframeList.keyframesIndex + k;
				const EntityAnimationDefinitionKeyframe &keyframe = animDef.keyframes[keyframeIndex];
				maxAnimWidth = std::max(maxAnimWidth, keyframe.width);
				maxAnimHeight = std::max(maxAnimHeight, keyframe.height);
			}
		}
	}

	*outMaxWidth = maxAnimWidth;
	*outMaxHeight = maxAnimHeight;
}

double EntityUtils::getCenterY(double feetY, double bboxHeight)
{
	return feetY + (bboxHeight * 0.50);
}

bool EntityUtils::tryGetDisplayName(const EntityDefinition &entityDef,
	const CharacterClassLibrary &charClassLibrary, std::string *outName)
{
	const EntityDefinitionType type = entityDef.getType();
	const bool isEnemy = type == EntityDefinitionType::Enemy;
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

bool EntityUtils::withinHearingDistance(const CoordDouble3 &listenerCoord, const CoordDouble2 &soundCoord, double ceilingScale)
{
	const CoordDouble3 soundCoord3D(
		soundCoord.chunk,
		VoxelDouble3(soundCoord.point.x, ceilingScale * 1.50, soundCoord.point.y));
	const VoxelDouble3 diff = soundCoord3D - listenerCoord;
	constexpr double hearingDistanceSqr = EntityUtils::HearingDistance * EntityUtils::HearingDistance;
	return diff.lengthSquared() < hearingDistanceSqr;
}

double EntityUtils::nextCreatureSoundWaitTime(Random &random)
{
	// Arbitrary amount of time.
	return 2.75 + (random.nextReal() * 4.50);
}
