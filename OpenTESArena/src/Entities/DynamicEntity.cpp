#include <algorithm>
#include <cmath>

#include "DynamicEntity.h"
#include "EntityManager.h"
#include "EntityType.h"
#include "../Game/CardinalDirection.h"
#include "../Game/CardinalDirectionName.h"
#include "../Game/Game.h"
#include "../Math/Constants.h"
#include "../Math/Quaternion.h"
#include "../Math/Random.h"
#include "../Math/RandomUtils.h"
#include "../Media/AudioManager.h"

#include "components/utilities/String.h"

namespace
{
	// Arbitrary value for how far away a creature can be heard from.
	// @todo: make this be part of the player, not creatures.
	constexpr double HearingDistance = 6.0;

	// How far away a citizen will consider idling around the player.
	constexpr double CitizenIdleDistance = 1.25;

	// Walking speed of citizens.
	constexpr double CitizenSpeed = 2.25;

	// Allowed directions for citizens to walk.
	const std::array<std::pair<CardinalDirectionName, NewDouble2>, 4> CitizenDirections =
	{
		{
			{ CardinalDirectionName::North, CardinalDirection::North },
			{ CardinalDirectionName::East, CardinalDirection::East },
			{ CardinalDirectionName::South, CardinalDirection::South },
			{ CardinalDirectionName::West, CardinalDirection::West }
		}
	};

	bool TryGetCitizenDirectionFromCardinalDirection(CardinalDirectionName directionName,
		NewDouble2 *outDirection)
	{
		const auto iter = std::find_if(CitizenDirections.begin(), CitizenDirections.end(),
			[directionName](const auto &pair)
		{
			return pair.first == directionName;
		});

		if (iter != CitizenDirections.end())
		{
			*outDirection = iter->second;
			return true;
		}
		else
		{
			return false;
		}
	}

	int GetRandomCitizenDirectionIndex(Random &random)
	{
		return random.next() % static_cast<int>(CitizenDirections.size());
	}
}

DynamicEntity::DynamicEntity()
	: direction(Double2::Zero), velocity(Double2::Zero)
{
	this->secondsTillCreatureSound = 0.0;
	this->derivedType = static_cast<DynamicEntityType>(-1);
}

void DynamicEntity::initCitizen(EntityDefID defID, const EntityAnimationInstance &animInst,
	CardinalDirectionName direction)
{
	this->init(defID, animInst);
	this->derivedType = DynamicEntityType::Citizen;

	if (!TryGetCitizenDirectionFromCardinalDirection(direction, &this->direction))
	{
		DebugCrash("Couldn't get citizen direction for \"" +
			std::to_string(static_cast<int>(direction)) + "\".");
	}
}

void DynamicEntity::initCreature(EntityDefID defID, const EntityAnimationInstance &animInst,
	const NewDouble2 &direction, Random &random)
{
	this->init(defID, animInst);
	this->derivedType = DynamicEntityType::Creature;
	this->direction = direction;
	this->secondsTillCreatureSound = DynamicEntity::nextCreatureSoundWaitTime(random);
}

void DynamicEntity::initProjectile(EntityDefID defID, const EntityAnimationInstance &animInst,
	const NewDouble2 &direction)
{
	this->init(defID, animInst);
	this->derivedType = DynamicEntityType::Projectile;
	this->direction = direction;
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

bool DynamicEntity::withinHearingDistance(const CoordDouble3 &point, double ceilingHeight)
{
	const CoordDouble3 position3D(
		this->position.chunk,
		VoxelDouble3(this->position.point.x, ceilingHeight * 1.50, this->position.point.y));
	const VoxelDouble3 diff = point - position3D;
	constexpr double hearingDistanceSqr = HearingDistance * HearingDistance;
	return diff.lengthSquared() < hearingDistanceSqr;
}

bool DynamicEntity::tryGetCreatureSoundFilename(const EntityManager &entityManager,
	const EntityDefinitionLibrary &entityDefLibrary, std::string *outFilename) const
{
	if (this->derivedType != DynamicEntityType::Creature)
	{
		return false;
	}

	const EntityDefinition &entityDef = entityManager.getEntityDef(
		this->getDefinitionID(), entityDefLibrary);
	if (entityDef.getType() != EntityDefinition::Type::Enemy)
	{
		return false;
	}

	const auto &enemyDef = entityDef.getEnemy();
	if (enemyDef.getType() != EntityDefinition::EnemyDefinition::Type::Creature)
	{
		return false;
	}

	const auto &creatureDef = enemyDef.getCreature();
	const std::string_view creatureSoundName = creatureDef.soundName;
	*outFilename = String::toUppercase(std::string(creatureSoundName));
	return true;
}

void DynamicEntity::playCreatureSound(const std::string &soundFilename, double ceilingHeight,
	AudioManager &audioManager)
{
	// Centered inside the creature.
	const CoordDouble3 soundCoord(
		this->position.chunk,
		VoxelDouble3(this->position.point.x, ceilingHeight * 1.50, this->position.point.y));
	const NewDouble3 absoluteSoundPosition = VoxelUtils::coordToNewPoint(soundCoord);
	audioManager.playSound(soundFilename, absoluteSoundPosition);
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

void DynamicEntity::lookAt(const CoordDouble2 &point)
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
	auto &gameData = game.getGameData();
	auto &random = game.getRandom();
	const auto &player = gameData.getPlayer();
	const auto &worldData = gameData.getActiveWorld();
	const auto &levelData = worldData.getActiveLevel();
	const auto &voxelGrid = levelData.getVoxelGrid();
	const auto &entityManager = levelData.getEntityManager();
	const auto &entityDefLibrary = game.getEntityDefinitionLibrary();
	const EntityDefinition &entityDef = entityManager.getEntityDef(
		this->getDefinitionID(), entityDefLibrary);
	const EntityAnimationDefinition &animDef = entityDef.getAnimDef();

	// Distance to player is used for switching animation states.
	const CoordDouble3 &playerPosition = player.getPosition();
	const CoordDouble2 playerPositionXZ(
		playerPosition.chunk, VoxelDouble2(playerPosition.point.x, playerPosition.point.z));
	const VoxelDouble2 dirToPlayer = playerPositionXZ - this->position;
	const double distToPlayerSqr = dirToPlayer.lengthSquared();

	// Get idle and walk state indices.
	int idleStateIndex, walkStateIndex;
	if (!animDef.tryGetStateIndex(EntityAnimationUtils::STATE_IDLE.c_str(), &idleStateIndex))
	{
		DebugLogWarning("Couldn't get citizen idle state index.");
		return;
	}

	if (!animDef.tryGetStateIndex(EntityAnimationUtils::STATE_WALK.c_str(), &walkStateIndex))
	{
		DebugLogWarning("Couldn't get citizen walk state index.");
		return;
	}

	constexpr double citizenIdleDistSqr = CitizenIdleDistance * CitizenIdleDistance;
	const auto &playerWeaponAnim = player.getWeaponAnimation();
	EntityAnimationInstance &animInst = this->getAnimInstance();
	const int curAnimStateIndex = animInst.getStateIndex();

	if (curAnimStateIndex == idleStateIndex)
	{
		const bool shouldChangeToWalking = !playerWeaponAnim.isSheathed() ||
			(distToPlayerSqr > citizenIdleDistSqr);

		// @todo: need to preserve their previous direction so they stay aligned with
		// the center of the voxel. Basically need to store cardinal direction as internal state.
		if (shouldChangeToWalking)
		{
			animInst.setStateIndex(walkStateIndex);
			const int citizenDirectionIndex = GetRandomCitizenDirectionIndex(random);
			const auto &citizenDirection = CitizenDirections[citizenDirectionIndex];
			this->direction = citizenDirection.second;
			this->velocity = this->direction * CitizenSpeed;
		}
		else
		{
			// Face towards player.
			this->setDirection(dirToPlayer);
		}
	}
	else if (curAnimStateIndex == walkStateIndex)
	{
		const bool shouldChangeToIdle = playerWeaponAnim.isSheathed() &&
			(distToPlayerSqr <= citizenIdleDistSqr);

		if (shouldChangeToIdle)
		{
			animInst.setStateIndex(idleStateIndex);
			this->velocity = NewDouble2::Zero;
		}
	}
}

void DynamicEntity::updateCreatureState(Game &game, double dt)
{
	auto &gameData = game.getGameData();
	const auto &worldData = gameData.getActiveWorld();
	const auto &levelData = worldData.getActiveLevel();
	const auto &entityManager = levelData.getEntityManager();
	const auto &entityDefLibrary = game.getEntityDefinitionLibrary();
	const double ceilingHeight = levelData.getCeilingHeight();

	// @todo: creature AI

	// Tick down the NPC's creature sound (if any). This is done on the top level so the counter
	// doesn't predictably begin when the player enters the creature's hearing distance.
	this->secondsTillCreatureSound -= dt;
	if (this->secondsTillCreatureSound <= 0.0)
	{
		// See if the NPC is withing hearing distance of the player.
		const CoordDouble3 &playerPosition = gameData.getPlayer().getPosition();
		if (this->withinHearingDistance(playerPosition, ceilingHeight))
		{
			// See if the NPC has a creature sound.
			std::string creatureSoundFilename;
			if (this->tryGetCreatureSoundFilename(entityManager, entityDefLibrary, &creatureSoundFilename))
			{
				auto &audioManager = game.getAudioManager();
				this->playCreatureSound(creatureSoundFilename, ceilingHeight, audioManager);

				const double creatureSoundWaitTime =
					DynamicEntity::nextCreatureSoundWaitTime(game.getRandom());
				this->secondsTillCreatureSound = creatureSoundWaitTime;
			}
		}
	}
}

void DynamicEntity::updateProjectileState(Game &game, double dt)
{
	// @todo: projectile motion + collision
}

void DynamicEntity::updatePhysics(const WorldData &worldData,
	const EntityDefinitionLibrary &entityDefLibrary, Random &random, double dt)
{
	const auto &levelData = worldData.getActiveLevel();
	const DynamicEntityType dynamicEntityType = this->getDerivedType();

	if (dynamicEntityType == DynamicEntityType::Citizen)
	{
		// Update citizen position and change facing if about to hit something.
		const auto &voxelGrid = levelData.getVoxelGrid();
		const auto &entityManager = levelData.getEntityManager();
		const EntityDefinition &entityDef = entityManager.getEntityDef(
			this->getDefinitionID(), entityDefLibrary);
		const EntityAnimationDefinition &animDef = entityDef.getAnimDef();

		// If citizen and walking, continue walking until next block is not air.
		int walkStateIndex;
		if (!animDef.tryGetStateIndex(EntityAnimationUtils::STATE_WALK.c_str(), &walkStateIndex))
		{
			DebugLogWarning("Couldn't get citizen walk state index.");
			return;
		}

		const EntityAnimationInstance &animInst = this->getAnimInstance();
		const int curAnimStateIndex = animInst.getStateIndex();
		if (curAnimStateIndex == walkStateIndex)
		{
			// Integrate by delta time.
			this->position = this->position + (this->velocity * dt);

			const NewDouble2 absolutePosition = VoxelUtils::coordToNewPoint(this->getPosition());
			const NewDouble2 &direction = this->getDirection();

			auto getVoxelAtDistance = [&absolutePosition](const NewDouble2 &checkDist)
			{
				return NewInt2(
					static_cast<SNInt>(std::floor(absolutePosition.x + checkDist.x)),
					static_cast<WEInt>(std::floor(absolutePosition.y + checkDist.y)));
			};

			const NewInt2 curVoxel = VoxelUtils::pointToVoxel(absolutePosition);
			const NewInt2 nextVoxel = getVoxelAtDistance(direction * 0.50);

			if (nextVoxel != curVoxel)
			{
				auto isSuitableVoxel = [&voxelGrid](const NewInt2 &voxel)
				{
					auto isValidVoxel = [&voxelGrid](const NewInt2 &voxel)
					{
						return voxelGrid.coordIsValid(voxel.x, 1, voxel.y);
					};

					auto isPassableVoxel = [&voxelGrid](const NewInt2 &voxel)
					{
						const uint16_t voxelID = voxelGrid.getVoxel(voxel.x, 1, voxel.y);
						const VoxelDefinition &voxelDef = voxelGrid.getVoxelDef(voxelID);
						return voxelDef.type == ArenaTypes::VoxelType::None;
					};

					auto isWalkableVoxel = [&voxelGrid](const NewInt2 &voxel)
					{
						const uint16_t voxelID = voxelGrid.getVoxel(voxel.x, 0, voxel.y);
						const VoxelDefinition &voxelDef = voxelGrid.getVoxelDef(voxelID);
						return voxelDef.type == ArenaTypes::VoxelType::Floor;
					};

					return isValidVoxel(voxel) && isPassableVoxel(voxel) && isWalkableVoxel(voxel);
				};

				if (!isSuitableVoxel(nextVoxel))
				{
					// Need to change walking direction. Determine another safe route, or if
					// none exist, then stop walking.
					const CardinalDirectionName curDirectionName =
						CardinalDirection::getDirectionName(direction);

					// Shuffle citizen direction indices so they don't all switch to the same
					// direction every time.
					std::array<int, 4> randomDirectionIndices = { 0, 1, 2, 3 };
					RandomUtils::shuffle(randomDirectionIndices.data(),
						static_cast<int>(randomDirectionIndices.size()), random);

					const auto iter = std::find_if(randomDirectionIndices.begin(), randomDirectionIndices.end(),
						[&getVoxelAtDistance, &isSuitableVoxel, curDirectionName](int dirIndex)
					{
						// See if this is a valid direction to go in.
						const auto &directionPair = CitizenDirections[dirIndex];
						const CardinalDirectionName cardinalDirectionName = directionPair.first;
						if (cardinalDirectionName != curDirectionName)
						{
							const NewDouble2 &direction = directionPair.second;
							const NewInt2 voxel = getVoxelAtDistance(direction * 0.50);
							if (isSuitableVoxel(voxel))
							{
								return true;
							}
						}

						return false;
					});

					if (iter != randomDirectionIndices.end())
					{
						const auto &directionPair = CitizenDirections[*iter];
						const NewDouble2 &newDirection = directionPair.second;
						this->setDirection(newDirection);
						this->velocity = newDirection * CitizenSpeed;
					}
					else
					{
						// Couldn't find any valid direction.
						this->velocity = NewDouble2::Zero;
					}
				}
			}
		}
	}

	// @todo: other dynamic entity types
}

void DynamicEntity::reset()
{
	Entity::reset();
	this->derivedType = static_cast<DynamicEntityType>(-1);
	this->secondsTillCreatureSound = 0.0;
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
	const auto &worldData = game.getGameData().getActiveWorld();
	const auto &entityDefLibrary = game.getEntityDefinitionLibrary();
	this->updatePhysics(worldData, entityDefLibrary, game.getRandom(), dt);
}
