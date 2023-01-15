#include <algorithm>
#include <cmath>

#include "DynamicEntity.h"
#include "EntityManager.h"
#include "EntityType.h"
#include "../Audio/AudioManager.h"
#include "../Game/CardinalDirection.h"
#include "../Game/CardinalDirectionName.h"
#include "../Game/Game.h"
#include "../Math/Constants.h"
#include "../Math/Quaternion.h"
#include "../Math/Random.h"
#include "../Math/RandomUtils.h"

#include "components/utilities/String.h"

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

	if (!CitizenUtils::tryGetCitizenDirectionFromCardinalDirection(direction, &this->direction))
	{
		DebugCrash("Couldn't get citizen direction for \"" + std::to_string(static_cast<int>(direction)) + "\".");
	}
}

void DynamicEntity::initCreature(EntityDefID defID, const EntityAnimationInstance &animInst,
	const WorldDouble2 &direction, Random &random)
{
	this->init(defID, animInst);
	this->derivedType = DynamicEntityType::Creature;
	this->direction = direction;
	this->secondsTillCreatureSound = DynamicEntity::nextCreatureSoundWaitTime(random);
}

void DynamicEntity::initProjectile(EntityDefID defID, const EntityAnimationInstance &animInst,
	const WorldDouble2 &direction)
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

const WorldDouble2 &DynamicEntity::getDirection() const
{
	return this->direction;
}

const WorldDouble2 &DynamicEntity::getVelocity() const
{
	return this->velocity;
}

const WorldDouble2 *DynamicEntity::getDestination() const
{
	return this->destination.has_value() ? &this->destination.value() : nullptr;
}

void DynamicEntity::setDirection(const WorldDouble2 &direction)
{
	DebugAssert(std::isfinite(direction.lengthSquared()));
	this->direction = direction;
}

double DynamicEntity::nextCreatureSoundWaitTime(Random &random)
{
	// Arbitrary amount of time.
	return 2.75 + (random.nextReal() * 4.50);
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

void DynamicEntity::playCreatureSound(const std::string &soundFilename, double ceilingScale,
	AudioManager &audioManager)
{
	// Centered inside the creature.
	const CoordDouble3 soundCoord(
		this->position.chunk,
		VoxelDouble3(this->position.point.x, ceilingScale * 1.50, this->position.point.y));
	const WorldDouble3 absoluteSoundPosition = VoxelUtils::coordToWorldPoint(soundCoord);
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
	this->direction = WorldDouble2(q.x, q.z).normalized();
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
	const WorldDouble2 newDirection = (point - this->position).normalized();

	// Only accept the change if it's valid.
	if (std::isfinite(newDirection.lengthSquared()))
	{
		this->direction = newDirection;
	}
}

void DynamicEntity::setDestination(const WorldDouble2 *point, double minDistance)
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

void DynamicEntity::setDestination(const WorldDouble2 *point)
{
	constexpr double minDistance = Constants::Epsilon;
	this->setDestination(point, minDistance);
}

void DynamicEntity::updateCitizenState(Game &game, double dt)
{
	const auto &player = game.getPlayer();
	auto &random = game.getRandom();
	auto &gameState = game.getGameState();
	const MapInstance &activeMapInst = gameState.getActiveMapInst();
	const LevelInstance &activeLevelInst = activeMapInst.getActiveLevel();
	const auto &entityManager = activeLevelInst.getEntityManager();
	const auto &entityDefLibrary = game.getEntityDefinitionLibrary();
	const EntityDefinition &entityDef = entityManager.getEntityDef(
		this->getDefinitionID(), entityDefLibrary);
	const EntityAnimationDefinition &animDef = entityDef.getAnimDef();

	// Distance to player and player velocity are used for switching animation states.
	const CoordDouble3 &playerPosition = player.getPosition();
	const CoordDouble2 playerPositionXZ(
		playerPosition.chunk, VoxelDouble2(playerPosition.point.x, playerPosition.point.z));
	const VoxelDouble2 dirToPlayer = playerPositionXZ - this->position;
	const double distToPlayerSqr = dirToPlayer.lengthSquared();

	const VoxelDouble3 &playerVelocity = player.getVelocity();
	const double playerSpeedSqr = playerVelocity.lengthSquared();
	const bool isPlayerStopped = playerSpeedSqr < Constants::Epsilon;

	// Get idle and walk state indices.
	const std::optional<int> idleStateIndex = animDef.tryGetStateIndex(EntityAnimationUtils::STATE_IDLE.c_str());
	if (!idleStateIndex.has_value())
	{
		DebugLogWarning("Couldn't get citizen idle state index.");
		return;
	}

	const std::optional<int> walkStateIndex = animDef.tryGetStateIndex(EntityAnimationUtils::STATE_WALK.c_str());
	if (!walkStateIndex.has_value())
	{
		DebugLogWarning("Couldn't get citizen walk state index.");
		return;
	}

	constexpr double citizenIdleDistSqr = CitizenUtils::IDLE_DISTANCE * CitizenUtils::IDLE_DISTANCE;
	const auto &playerWeaponAnim = player.getWeaponAnimation();
	EntityAnimationInstance &animInst = this->getAnimInstance();
	const int curAnimStateIndex = animInst.getStateIndex();

	if (curAnimStateIndex == idleStateIndex)
	{
		const bool shouldChangeToWalking = !playerWeaponAnim.isSheathed() ||
			(distToPlayerSqr > citizenIdleDistSqr) || !isPlayerStopped;

		// @todo: need to preserve their previous direction so they stay aligned with
		// the center of the voxel. Basically need to store cardinal direction as internal state.
		if (shouldChangeToWalking)
		{
			animInst.setStateIndex(*walkStateIndex);
			const int citizenDirectionIndex = CitizenUtils::getRandomCitizenDirectionIndex(random);
			this->direction = CitizenUtils::getCitizenDirectionByIndex(citizenDirectionIndex);
			this->velocity = this->direction * CitizenUtils::SPEED;
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
			(distToPlayerSqr <= citizenIdleDistSqr) && isPlayerStopped;

		if (shouldChangeToIdle)
		{
			animInst.setStateIndex(*idleStateIndex);
			this->velocity = WorldDouble2::Zero;
		}
	}
}

void DynamicEntity::updateCreatureState(Game &game, double dt)
{
	auto &gameState = game.getGameState();
	const MapInstance &activeMapInst = gameState.getActiveMapInst();
	const LevelInstance &activeLevelInst = activeMapInst.getActiveLevel();
	const auto &entityManager = activeLevelInst.getEntityManager();
	const auto &entityDefLibrary = game.getEntityDefinitionLibrary();
	const double ceilingScale = activeLevelInst.getCeilingScale();

	// @todo: creature AI

	// Tick down the NPC's creature sound (if any). This is done on the top level so the counter
	// doesn't predictably begin when the player enters the creature's hearing distance.
	this->secondsTillCreatureSound -= dt;
	if (this->secondsTillCreatureSound <= 0.0)
	{
		// See if the NPC is withing hearing distance of the player.
		const CoordDouble3 &playerPosition = game.getPlayer().getPosition();
		if (EntityUtils::withinHearingDistance(playerPosition, this->position, ceilingScale))
		{
			// See if the NPC has a creature sound.
			std::string creatureSoundFilename;
			if (this->tryGetCreatureSoundFilename(entityManager, entityDefLibrary, &creatureSoundFilename))
			{
				auto &audioManager = game.getAudioManager();
				this->playCreatureSound(creatureSoundFilename, ceilingScale, audioManager);

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

void DynamicEntity::updatePhysics(const LevelInstance &activeLevel,
	const EntityDefinitionLibrary &entityDefLibrary, Random &random, double dt)
{
	const VoxelChunkManager &voxelChunkManager = activeLevel.getVoxelChunkManager();
	const EntityManager &entityManager = activeLevel.getEntityManager();
	const DynamicEntityType dynamicEntityType = this->getDerivedType();

	if (dynamicEntityType == DynamicEntityType::Citizen)
	{
		// Update citizen position and change facing if about to hit something.
		const EntityDefinition &entityDef = entityManager.getEntityDef(
			this->getDefinitionID(), entityDefLibrary);
		const EntityAnimationDefinition &animDef = entityDef.getAnimDef();

		// If citizen and walking, continue walking until next block is not air.
		const std::optional<int> walkStateIndex = animDef.tryGetStateIndex(EntityAnimationUtils::STATE_WALK.c_str());
		if (!walkStateIndex.has_value())
		{
			DebugLogWarning("Couldn't get citizen walk state index.");
			return;
		}

		const EntityAnimationInstance &animInst = this->getAnimInstance();
		const int curAnimStateIndex = animInst.getStateIndex();
		if (curAnimStateIndex == *walkStateIndex)
		{
			// Integrate by delta time.
			this->position = this->position + (this->velocity * dt);

			const CoordDouble2 newPosition = this->getPosition();
			const VoxelDouble2 &direction = this->getDirection();

			auto getVoxelAtDistance = [&newPosition](const VoxelDouble2 &checkDist) -> CoordInt2
			{
				const CoordDouble2 pos = newPosition + checkDist;
				return CoordInt2(pos.chunk, VoxelUtils::pointToVoxel(pos.point));
			};

			const CoordInt2 curVoxel(newPosition.chunk, VoxelUtils::pointToVoxel(newPosition.point));
			const CoordInt2 nextVoxel = getVoxelAtDistance(direction * 0.50);

			if (nextVoxel != curVoxel)
			{
				auto isSuitableVoxel = [&voxelChunkManager](const CoordInt2 &coord)
				{
					const VoxelChunk *chunk = voxelChunkManager.tryGetChunkAtPosition(coord.chunk);

					auto isValidVoxel = [chunk]()
					{
						return chunk != nullptr;
					};

					auto isPassableVoxel = [&coord, chunk]()
					{
						const VoxelInt3 voxel(coord.voxel.x, 1, coord.voxel.y);
						const VoxelChunk::VoxelTraitsDefID voxelTraitsDefID = chunk->getTraitsDefID(voxel.x, voxel.y, voxel.z);
						const VoxelTraitsDefinition &voxelTraitsDef = chunk->getTraitsDef(voxelTraitsDefID);
						return voxelTraitsDef.type == ArenaTypes::VoxelType::None;
					};

					auto isWalkableVoxel = [&coord, chunk]()
					{
						const VoxelInt3 voxel(coord.voxel.x, 0, coord.voxel.y);
						const VoxelChunk::VoxelTraitsDefID voxelTraitsDefID = chunk->getTraitsDefID(voxel.x, voxel.y, voxel.z);
						const VoxelTraitsDefinition &voxelTraitsDef = chunk->getTraitsDef(voxelTraitsDefID);
						return voxelTraitsDef.type == ArenaTypes::VoxelType::Floor;
					};

					return isValidVoxel() && isPassableVoxel() && isWalkableVoxel();
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
						const CardinalDirectionName cardinalDirectionName =
							CitizenUtils::getCitizenDirectionNameByIndex(dirIndex);
						if (cardinalDirectionName != curDirectionName)
						{
							const WorldDouble2 &direction = CitizenUtils::getCitizenDirectionByIndex(dirIndex);
							const CoordInt2 voxel = getVoxelAtDistance(direction * 0.50);
							if (isSuitableVoxel(voxel))
							{
								return true;
							}
						}

						return false;
					});

					if (iter != randomDirectionIndices.end())
					{
						const WorldDouble2 &newDirection = CitizenUtils::getCitizenDirectionByIndex(*iter);
						this->setDirection(newDirection);
						this->velocity = newDirection * CitizenUtils::SPEED;
					}
					else
					{
						// Couldn't find any valid direction.
						this->velocity = WorldDouble2::Zero;
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
	const GameState &gameState = game.getGameState();
	const MapInstance &activeMapInst = gameState.getActiveMapInst();
	const LevelInstance &activeLevelInst = activeMapInst.getActiveLevel();
	const EntityDefinitionLibrary &entityDefLibrary = game.getEntityDefinitionLibrary();
	this->updatePhysics(activeLevelInst, entityDefLibrary, game.getRandom(), dt);
}
