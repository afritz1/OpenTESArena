#include <algorithm>
#include <cmath>
#include <vector>

#include "CharacterClassDefinition.h"
#include "CharacterClassLibrary.h"
#include "Player.h"
#include "PrimaryAttributeName.h"
#include "../Collision/CollisionChunk.h"
#include "../Game/CardinalDirection.h"
#include "../Game/Game.h"
#include "../Game/GameState.h"
#include "../Game/Options.h"
#include "../Math/Constants.h"
#include "../Math/Random.h"

#include "components/debug/Debug.h"
#include "components/utilities/Buffer.h"
#include "components/utilities/String.h"

namespace // @todo: could be in a PlayerUtils instead
{
	constexpr double STEPPING_HEIGHT = 0.25; // Allowed change in height for stepping on stairs.
	constexpr double JUMP_VELOCITY = 3.0; // Instantaneous change in Y velocity when jumping.

	// Magnitude of -Y acceleration in the air.
	constexpr double GRAVITY = 9.81;

	// Friction for slowing the player down on ground.
	constexpr double FRICTION_DYNAMIC = 4.0;
	constexpr double FRICTION_STATIC = 16.0;
}

Player::Player()
{
	this->male = false;
	this->raceID = -1;
	this->charClassDefID = -1;
	this->portraitID = -1;
	this->camera.init(CoordDouble3(), -Double3::UnitX); // To avoid audio listener normalization issues w/ uninitialized player.
	this->maxWalkSpeed = 0.0;
	this->maxRunSpeed = 0.0;
	this->friction = 0.0;
}

void Player::init(const std::string &displayName, bool male, int raceID, int charClassDefID,
	int portraitID, const CoordDouble3 &position, const Double3 &direction, const Double3 &velocity,
	double maxWalkSpeed, double maxRunSpeed, int weaponID, const ExeData &exeData, Random &random)
{
	this->displayName = displayName;
	this->male = male;
	this->raceID = raceID;
	this->charClassDefID = charClassDefID;
	this->portraitID = portraitID;
	this->camera.init(position, direction);
	this->velocity = velocity;
	this->maxWalkSpeed = maxWalkSpeed;
	this->maxRunSpeed = maxRunSpeed;
	this->friction = FRICTION_STATIC;
	this->weaponAnimation.init(weaponID, exeData);
	this->attributes.init(raceID, male, random);
}

void Player::init(const std::string &displayName, bool male, int raceID, int charClassDefID,
	PrimaryAttributeSet &&attributes, int portraitID, const CoordDouble3 &position, const Double3 &direction,
	const Double3 &velocity, double maxWalkSpeed, double maxRunSpeed, int weaponID, const ExeData &exeData)
{
	this->displayName = displayName;
	this->male = male;
	this->raceID = raceID;
	this->charClassDefID = charClassDefID;
	this->portraitID = portraitID;
	this->camera.init(position, direction);
	this->velocity = velocity;
	this->maxWalkSpeed = maxWalkSpeed;
	this->maxRunSpeed = maxRunSpeed;
	this->friction = FRICTION_STATIC;
	this->weaponAnimation.init(weaponID, exeData);
	this->attributes = std::move(attributes);
}

void Player::initRandom(const CharacterClassLibrary &charClassLibrary, const ExeData &exeData, Random &random)
{
	this->displayName = "Player";
	this->male = random.next(2) == 0;
	this->raceID = random.next(8);
	this->charClassDefID = random.next(charClassLibrary.getDefinitionCount());
	this->portraitID = random.next(10);

	const CoordDouble3 position(ChunkInt2::Zero, VoxelDouble3::Zero);
	const Double3 direction(CardinalDirection::North.x, 0.0, CardinalDirection::North.y);
	this->camera.init(position, direction);
	this->velocity = Double3::Zero;
	this->maxWalkSpeed = Player::DEFAULT_WALK_SPEED;
	this->maxRunSpeed = Player::DEFAULT_RUN_SPEED;

	const CharacterClassDefinition &charClassDef = charClassLibrary.getDefinition(this->charClassDefID);
	const int weaponID = [&random, &charClassDef]()
	{
		// Generate weapons available for this class and pick a random one.
		const int allowedWeaponCount = charClassDef.getAllowedWeaponCount();
		Buffer<int> weapons(allowedWeaponCount + 1);
		for (int i = 0; i < allowedWeaponCount; i++)
		{
			weapons.set(i, charClassDef.getAllowedWeapon(i));
		}

		// Add fists.
		weapons.set(allowedWeaponCount, -1);

		const int randIndex = random.next(weapons.getCount());
		return weapons.get(randIndex);
	}();

	this->weaponAnimation.init(weaponID, exeData);
	this->attributes.init(this->raceID, this->male, random);
}

const CoordDouble3 &Player::getPosition() const
{
	return this->camera.position;
}

const std::string &Player::getDisplayName() const
{
	return this->displayName;
}

std::string Player::getFirstName() const
{
	std::vector<std::string> nameTokens = String::split(this->displayName);
	return nameTokens.at(0);
}

int Player::getPortraitID() const
{
	return this->portraitID;
}

bool Player::isMale() const
{
	return this->male;
}

int Player::getRaceID() const
{
	return this->raceID;
}

int Player::getCharacterClassDefID() const
{
	return this->charClassDefID;
}

const PrimaryAttributeSet &Player::getAttributes() const
{
	return this->attributes;
}

const Double3 &Player::getDirection() const
{
	return this->camera.getDirection();
}

const Double3 &Player::getRight() const
{
	return this->camera.getRight();
}

Double2 Player::getGroundDirection() const
{
	const Double3 &direction = this->camera.getDirection();
	return Double2(direction.x, direction.z).normalized();
}

const VoxelDouble3 &Player::getVelocity() const
{
	return this->velocity;
}

double Player::getJumpMagnitude() const
{
	return JUMP_VELOCITY;
}

WeaponAnimation &Player::getWeaponAnimation()
{
	return this->weaponAnimation;
}

const WeaponAnimation &Player::getWeaponAnimation() const
{
	return this->weaponAnimation;
}

double Player::getFeetY() const
{
	const double cameraY = this->camera.position.point.y;
	return cameraY - Player::HEIGHT;
}

bool Player::onGround(const LevelInstance &activeLevel) const
{
	// @todo: find a non-hack way to do this.

	return true;

	// This function seems kind of like a hack right now, since the player's feet
	// will frequently be at Y == 1.0, which is one voxel above the ground, and
	// it won't be considered as "on ground" unless it checks the voxel underneath
	// of this particular Y position (due to the rounding rules being used).
	/*const double feetY = this->getFeetY();
	const double feetVoxelYPos = std::floor(feetY);
	const bool closeEnoughToLowerVoxel = std::abs(feetY - feetVoxelYPos) < EPSILON;
	const WorldInt3 feetVoxel(
		static_cast<int>(std::floor(this->camera.position.x)),
		static_cast<int>(feetVoxelYPos) - (closeEnoughToLowerVoxel ? 1 : 0),
		static_cast<int>(std::floor(this->camera.position.z)));

	const bool insideWorld = [&feetVoxel, &voxelGrid]()
	{
		return (feetVoxel.x >= 0) && (feetVoxel.x < voxelGrid.getWidth()) &&
			(feetVoxel.y >= 0) && (feetVoxel.y < voxelGrid.getHeight()) &&
			(feetVoxel.z >= 0) && (feetVoxel.z < voxelGrid.getDepth());
	}();

	// Don't try to dereference the voxel grid if the player's feet are outside.
	if (insideWorld)
	{
		const char feetVoxelID = voxelGrid.getVoxels()[feetVoxel.x +
			(feetVoxel.y * voxelGrid.getWidth()) +
			(feetVoxel.z * voxelGrid.getWidth() * voxelGrid.getHeight())];
		const VoxelData &voxelData = voxelGrid.getVoxelData(feetVoxelID);

		return (this->velocity.y == 0.0) && !voxelData.isAir() &&
			(feetY >= (feetVoxelYPos + voxelData.yOffset)) &&
			(feetY <= (feetVoxelYPos + voxelData.yOffset + voxelData.ySize));
	}
	else return false;*/
}

void Player::teleport(const CoordDouble3 &position)
{
	this->camera.position = position;
}

void Player::rotate(double dx, double dy, double hSensitivity, double vSensitivity,
	double pitchLimit)
{
	// Multiply sensitivities by 100 so the values in the options are nicer.
	this->camera.rotate(dx * (100.0 * hSensitivity), dy * (100.0 * vSensitivity), pitchLimit);
}

void Player::lookAt(const CoordDouble3 &point)
{
	this->camera.lookAt(point);
}

void Player::handleCollision(const LevelInstance &activeLevel, double dt)
{
	const VoxelChunkManager &voxelChunkManager = activeLevel.getVoxelChunkManager();
	const CollisionChunkManager &collisionChunkManager = activeLevel.getCollisionChunkManager();

	auto tryGetVoxelTraitsDef = [&activeLevel, &voxelChunkManager](const CoordInt3 &coord) -> const VoxelTraitsDefinition*
	{
		const VoxelChunk *chunk = voxelChunkManager.tryGetChunkAtPosition(coord.chunk);
		if (chunk != nullptr)
		{
			const VoxelChunk::VoxelTraitsDefID voxelTraitsDefID = chunk->getTraitsDefID(coord.voxel.x, coord.voxel.y, coord.voxel.z);
			const VoxelTraitsDefinition &voxelTraitsDef = chunk->getTraitsDef(voxelTraitsDefID);
			return &voxelTraitsDef;
		}
		else
		{
			// Chunks not in the chunk manager are air.
			return nullptr;
		}
	};

	// Coordinates of the base of the voxel the feet are in.
	// - @todo: add delta velocity Y?
	const int feetVoxelY = static_cast<int>(std::floor(this->getFeetY() / activeLevel.getCeilingScale()));

	// Regular old Euler integration in XZ plane.
	const CoordDouble3 curPlayerCoord = this->getPosition();
	const VoxelDouble3 deltaPosition(this->velocity.x * dt, 0.0, this->velocity.z * dt);

	// The next voxels in X/Y/Z directions based on player movement.
	const VoxelInt3 nextXVoxel(
		static_cast<SNInt>(std::floor(curPlayerCoord.point.x + deltaPosition.x)),
		feetVoxelY,
		static_cast<WEInt>(std::floor(curPlayerCoord.point.z)));
	const VoxelInt3 nextYVoxel(
		static_cast<SNInt>(std::floor(curPlayerCoord.point.x)),
		nextXVoxel.y,
		nextXVoxel.z);
	const VoxelInt3 nextZVoxel(
		nextYVoxel.x,
		nextYVoxel.y,
		static_cast<WEInt>(std::floor(curPlayerCoord.point.z + deltaPosition.z)));

	const CoordInt3 nextXCoord = ChunkUtils::recalculateCoord(curPlayerCoord.chunk, nextXVoxel);
	const CoordInt3 nextYCoord = ChunkUtils::recalculateCoord(curPlayerCoord.chunk, nextYVoxel);
	const CoordInt3 nextZCoord = ChunkUtils::recalculateCoord(curPlayerCoord.chunk, nextZVoxel);
	const VoxelTraitsDefinition *xVoxelTraitsDef = tryGetVoxelTraitsDef(nextXCoord);
	const VoxelTraitsDefinition *yVoxelTraitsDef = tryGetVoxelTraitsDef(nextYCoord);
	const VoxelTraitsDefinition *zVoxelTraitsDef = tryGetVoxelTraitsDef(nextZCoord);

	// Check horizontal collisions.

	// -- Temp hack until Y collision detection is implemented --
	// - @todo: formalize the collision calculation and get rid of this hack.
	//   We should be able to cover all collision cases in Arena now.
	auto wouldCollideWithVoxel = [&voxelChunkManager, &collisionChunkManager](const CoordInt3 &coord, const VoxelTraitsDefinition &voxelTraitsDef)
	{
		const ArenaTypes::VoxelType voxelType = voxelTraitsDef.type;

		if (voxelType == ArenaTypes::VoxelType::TransparentWall)
		{
			const VoxelTraitsDefinition::TransparentWall &transparent = voxelTraitsDef.transparentWall;
			return transparent.collider;
		}
		else if (voxelType == ArenaTypes::VoxelType::Edge)
		{
			// Edge collision.
			// - @todo: treat as edge, not solid voxel.
			const VoxelTraitsDefinition::Edge &edge = voxelTraitsDef.edge;
			return edge.collider;
		}
		else
		{
			const ChunkInt2 &chunkPos = coord.chunk;
			const VoxelInt3 &voxelPos = coord.voxel;
			const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);
			const CollisionChunk &collisionChunk = collisionChunkManager.getChunkAtPosition(chunkPos);

			// General voxel collision.
			const CollisionChunk::CollisionMeshDefID collisionMeshDefID = collisionChunk.meshDefIDs.get(voxelPos.x, voxelPos.y, voxelPos.z);
			const CollisionMeshDefinition &collisionMeshDef = collisionChunk.getCollisionMeshDef(collisionMeshDefID);
			const bool isEmpty = collisionMeshDef.vertices.getCount() == 0;

			// @todo: don't check that it's a door, just check the collision chunk directly for everything (it should be data-driven, not type-driven).
			const bool isOpenDoor = [voxelType, &voxelPos, &collisionChunk]()
			{
				if (voxelType == ArenaTypes::VoxelType::Door)
				{
					return !collisionChunk.enabledColliders.get(voxelPos.x, voxelPos.y, voxelPos.z);
				}
				else
				{
					return false;
				}
			}();

			// -- Temporary hack for "on voxel enter" transitions --
			// - @todo: replace with "on would enter voxel" event and near facing check.
			const bool isLevelTransition = [&coord, voxelType, &voxelChunk]()
			{
				if (voxelType == ArenaTypes::VoxelType::Wall)
				{
					const VoxelInt3 &voxel = coord.voxel;

					// Check if there is a level change transition definition for this voxel.
					VoxelChunk::TransitionDefID transitionDefID;
					if (!voxelChunk.tryGetTransitionDefID(voxel.x, voxel.y, voxel.z, &transitionDefID))
					{
						return false;
					}

					const TransitionDefinition &transitionDef = voxelChunk.getTransitionDef(transitionDefID);
					return transitionDef.getType() == TransitionType::LevelChange;
				}
				else
				{
					return false;
				}
			}();

			return !isEmpty && !isOpenDoor && !isLevelTransition;
		}
	};

	if ((xVoxelTraitsDef != nullptr) && wouldCollideWithVoxel(nextXCoord, *xVoxelTraitsDef))
	{
		this->velocity.x = 0.0;
	}

	if ((zVoxelTraitsDef != nullptr) && wouldCollideWithVoxel(nextZCoord, *zVoxelTraitsDef))
	{
		this->velocity.z = 0.0;
	}

	this->velocity.y = 0.0;
	// -- end hack --

	// @todo: use an axis-aligned bounding box or cylinder instead of a point?
}

void Player::setVelocityToZero()
{
	this->velocity = Double3::Zero;
}

void Player::setFrictionToDynamic()
{
	this->friction = FRICTION_DYNAMIC;
}

void Player::setFrictionToStatic()
{
	this->friction = FRICTION_STATIC;
}

void Player::setDirectionToHorizon()
{
	const CoordDouble3 &coord = this->getPosition();
	const WorldDouble2 groundDirection = this->getGroundDirection();
	const VoxelDouble3 lookAtPoint = coord.point + VoxelDouble3(groundDirection.x, 0.0, groundDirection.y);
	const CoordDouble3 lookAtCoord(coord.chunk, lookAtPoint);
	this->lookAt(lookAtCoord);
}

void Player::accelerate(const Double3 &direction, double magnitude,
	bool isRunning, double dt)
{
	DebugAssert(dt >= 0.0);
	DebugAssert(magnitude >= 0.0);
	DebugAssert(std::isfinite(magnitude));
	DebugAssert(direction.isNormalized());

	// Simple Euler integration for updating velocity.
	Double3 newVelocity = this->velocity + (direction * (magnitude * dt));

	if (std::isfinite(newVelocity.length()))
	{
		this->velocity = newVelocity;
	}

	// Don't let the horizontal velocity be greater than the max speed for the
	// current movement state (i.e., walking/running).
	double maxSpeed = isRunning ? this->maxRunSpeed : this->maxWalkSpeed;
	Double2 velocityXZ(this->velocity.x, this->velocity.z);
	if (velocityXZ.length() > maxSpeed)
	{
		velocityXZ = velocityXZ.normalized() * maxSpeed;
	}

	// If the velocity is near zero, set it to zero. This fixes a problem where
	// the velocity could remain at a tiny magnitude and never reach zero.
	if (this->velocity.length() < 0.001)
	{
		this->velocity = Double3::Zero;
	}
}

void Player::accelerateInstant(const Double3 &direction, double magnitude)
{
	DebugAssert(direction.isNormalized());

	const Double3 additiveVelocity = direction * magnitude;

	if (std::isfinite(additiveVelocity.length()))
	{
		this->velocity = this->velocity + additiveVelocity;
	}
}

void Player::updatePhysics(const LevelInstance &activeLevel, bool collision, double dt)
{
	// Acceleration from gravity (always).
	this->accelerate(-Double3::UnitY, GRAVITY, false, dt);

	// Temp: get floor Y until Y collision is implemented.
	const double floorY = activeLevel.getCeilingScale();

	// Change the player's velocity based on collision.
	if (collision)
	{
		this->handleCollision(activeLevel, dt);

		// Temp: keep camera Y fixed until Y collision is implemented.
		this->camera.position.point.y = floorY + Player::HEIGHT;
	}
	else
	{
		// Keep the player's Y position constant, but otherwise let them act as a ghost.
		this->camera.position.point.y = floorY + Player::HEIGHT;
		this->velocity.y = 0.0;
	}

	// Simple Euler integration for updating the player's position.
	const VoxelDouble3 newPoint = this->camera.position.point + (this->velocity * dt);

	// Update the position if valid.
	if (std::isfinite(newPoint.length()))
	{
		this->camera.position = ChunkUtils::recalculateCoord(this->camera.position.chunk, newPoint);
	}

	if (this->onGround(activeLevel))
	{
		// Slow down the player's horizontal velocity with some friction.
		Double2 velocityXZ(this->velocity.x, this->velocity.z);
		Double2 frictionDirection = Double2(-velocityXZ.x, -velocityXZ.y).normalized();
		double frictionMagnitude = velocityXZ.length() * this->friction;

		if (std::isfinite(frictionDirection.length()) && (frictionMagnitude > Constants::Epsilon))
		{
			this->accelerate(Double3(frictionDirection.x, 0.0, frictionDirection.y),
				frictionMagnitude, true, dt);
		}
	}
}

void Player::tick(Game &game, double dt)
{
	// Update player position and velocity due to collisions.
	const GameState &gameState = game.getGameState();
	const MapInstance &activeMapInst = gameState.getActiveMapInst();
	const LevelInstance &activeLevelInst = activeMapInst.getActiveLevel();
	const bool collisionEnabled = game.getOptions().getMisc_Collision();
	this->updatePhysics(activeLevelInst, collisionEnabled, dt);

	// Tick weapon animation.
	this->weaponAnimation.tick(dt);
}
