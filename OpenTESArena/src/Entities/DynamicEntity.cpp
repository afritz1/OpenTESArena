#include <cmath>

#include "DynamicEntity.h"
#include "EntityManager.h"
#include "EntityType.h"
#include "../Game/Game.h"
#include "../Math/Constants.h"
#include "../Math/Quaternion.h"
#include "../Math/Random.h"
#include "../Media/AudioManager.h"

#include "components/utilities/String.h"

namespace
{
	// @todo: maybe want this to be a part of GameData? File-scope globals are not ideal.
	Random CreatureSoundRandom;

	// Arbitrary value for how far away a creature can be heard from.
	// @todo: make this be part of the player, not creatures.
	constexpr double HearingDistance = 6.0;
}

DynamicEntity::DynamicEntity()
	: direction(Double2::Zero), velocity(Double2::Zero)
{
	this->secondsTillCreatureSound = 0.0;
	this->derivedType = static_cast<DynamicEntityType>(-1);
}

EntityType DynamicEntity::getEntityType() const
{
	return EntityType::Dynamic;
}

DynamicEntityType DynamicEntity::getDerivedType() const
{
	return this->derivedType;
}

const NewDouble2 &DynamicEntity::getDirection() const
{
	return this->direction;
}

const NewDouble2 &DynamicEntity::getVelocity() const
{
	return this->velocity;
}

const NewDouble2 *DynamicEntity::getDestination() const
{
	return this->destination.has_value() ? &this->destination.value() : nullptr;
}

void DynamicEntity::setDerivedType(DynamicEntityType derivedType)
{
	this->derivedType = derivedType;
}

void DynamicEntity::setDirection(const NewDouble2 &direction)
{
	DebugAssert(std::isfinite(direction.lengthSquared()));
	this->direction = direction;
}

double DynamicEntity::nextCreatureSoundWaitTime(Random &random)
{
	// Arbitrary amount of time.
	return 2.75 + (random.nextReal() * 4.50);
}

bool DynamicEntity::withinHearingDistance(const Double3 &point, double ceilingHeight)
{
	const Double3 position3D(this->position.x, ceilingHeight * 1.50, this->position.y);
	return (point - position3D).lengthSquared() < (HearingDistance * HearingDistance);
}

bool DynamicEntity::tryGetCreatureSoundFilename(const EntityManager &entityManager,
	std::string *outFilename) const
{
	if (this->derivedType != DynamicEntityType::Creature)
	{
		return false;
	}

	const EntityDefinition &entityDef = entityManager.getEntityDef(this->getDefinitionID());
	if (!entityDef.isCreature())
	{
		return false;
	}

	const std::string creatureSoundName = entityDef.getCreatureData().soundName;
	*outFilename = String::toUppercase(creatureSoundName);
	return true;
}

void DynamicEntity::playCreatureSound(const std::string &soundFilename, double ceilingHeight,
	AudioManager &audioManager)
{
	// Centered inside the creature.
	const Double3 soundPosition(this->position.x, ceilingHeight * 1.50, this->position.y);
	audioManager.playSound(soundFilename, soundPosition);
}

void DynamicEntity::yaw(double radians)
{
	// Convert direction to 3D.
	const Double3 forward = Double3(this->direction.x, 0.0,
		this->direction.y).normalized();

	// Rotate around "global up".
	Quaternion q = Quaternion::fromAxisAngle(Double3::UnitY, radians) *
		Quaternion(forward, 0.0);

	// Convert back to 2D.
	this->direction = NewDouble2(q.x, q.z).normalized();
}

void DynamicEntity::rotate(double degrees)
{
	double lookRightRads = degrees * Constants::DegToRad;

	if (!std::isfinite(lookRightRads))
	{
		lookRightRads = 0.0;
	}

	this->yaw(-lookRightRads);
}

void DynamicEntity::lookAt(const NewDouble2 &point)
{
	const NewDouble2 newDirection = (point - this->position).normalized();

	// Only accept the change if it's valid.
	if (std::isfinite(newDirection.lengthSquared()))
	{
		this->direction = newDirection;
	}
}

void DynamicEntity::setDestination(const NewDouble2 *point, double minDistance)
{
	if (point != nullptr)
	{
		this->destination = *point;
	}
	else
	{
		this->destination = std::nullopt;
	}
}

void DynamicEntity::setDestination(const NewDouble2 *point)
{
	constexpr double minDistance = Constants::Epsilon;
	this->setDestination(point, minDistance);
}

void DynamicEntity::updateCitizenState(Game &game, double dt)
{
	// @todo: citizen AI
}

void DynamicEntity::updateCreatureState(Game &game, double dt)
{
	auto &gameData = game.getGameData();
	const auto &worldData = gameData.getWorldData();
	const auto &levelData = worldData.getActiveLevel();
	const auto &entityManager = levelData.getEntityManager();
	const double ceilingHeight = levelData.getCeilingHeight();

	// @todo: creature AI

	// Tick down the NPC's creature sound (if any). This is done on the top level so the counter
	// doesn't predictably begin when the player enters the creature's hearing distance.
	this->secondsTillCreatureSound -= dt;
	if (this->secondsTillCreatureSound <= 0.0)
	{
		// See if the NPC is withing hearing distance of the player.
		const Double3 &playerPosition = gameData.getPlayer().getPosition();
		if (this->withinHearingDistance(playerPosition, ceilingHeight))
		{
			// See if the NPC has a creature sound.
			std::string creatureSoundFilename;
			if (this->tryGetCreatureSoundFilename(entityManager, &creatureSoundFilename))
			{
				auto &audioManager = game.getAudioManager();
				this->playCreatureSound(creatureSoundFilename, ceilingHeight, audioManager);

				const double creatureSoundWaitTime =
					DynamicEntity::nextCreatureSoundWaitTime(CreatureSoundRandom);
				this->secondsTillCreatureSound = creatureSoundWaitTime;
			}
		}
	}
}

void DynamicEntity::updateProjectileState(Game &game, double dt)
{
	// @todo: projectile motion + collision
}

void DynamicEntity::updatePhysics(const WorldData &worldData, double dt)
{
	// @todo
}

void DynamicEntity::reset()
{
	Entity::reset();
	this->derivedType = static_cast<DynamicEntityType>(-1);
	this->secondsTillCreatureSound = DynamicEntity::nextCreatureSoundWaitTime(CreatureSoundRandom);
	this->destination = std::nullopt;
}

void DynamicEntity::tick(Game &game, double dt)
{
	Entity::tick(game, dt);

	// Update derived entity state.
	switch (this->derivedType)
	{
	case DynamicEntityType::Citizen:
		this->updateCitizenState(game, dt);
		break;
	case DynamicEntityType::Creature:
		this->updateCreatureState(game, dt);
		break;
	case DynamicEntityType::Projectile:
		this->updateProjectileState(game, dt);
		break;
	default:
		DebugNotImplementedMsg(std::to_string(static_cast<int>(this->derivedType)));
	}

	// Update physics/pathfinding/etc..
	// @todo: add a check here if updating the entity state has put them in a non-physics state.
	const auto &worldData = game.getGameData().getWorldData();
	this->updatePhysics(worldData, dt);
}
