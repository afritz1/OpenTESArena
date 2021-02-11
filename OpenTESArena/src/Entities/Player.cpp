#include <algorithm>
#include <cmath>
#include <vector>

#include "CharacterClassDefinition.h"
#include "CharacterClassLibrary.h"
#include "EntityType.h"
#include "Player.h"
#include "../Game/CardinalDirection.h"
#include "../Game/Game.h"
#include "../Game/GameData.h"
#include "../Game/Options.h"
#include "../Math/Constants.h"
#include "../Math/Random.h"
#include "../World/LevelData.h"
#include "../World/VoxelDefinition.h"
#include "../World/VoxelGrid.h"
#include "../World/WorldData.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

Player::Player(const std::string &displayName, bool male, int raceID, int charClassDefID,
	int portraitID, const CoordDouble3 &position, const Double3 &direction, const Double3 &velocity,
	double maxWalkSpeed, double maxRunSpeed, int weaponID, const ExeData &exeData)
	: displayName(displayName), male(male), raceID(raceID), charClassDefID(charClassDefID),
	portraitID(portraitID), camera(position, direction), velocity(velocity),
	maxWalkSpeed(maxWalkSpeed), maxRunSpeed(maxRunSpeed), weaponAnimation(weaponID, exeData) { }

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

Player Player::makeRandom(const CharacterClassLibrary &charClassLibrary,
	const ExeData &exeData, Random &random)
{
	const std::string name("Player");
	const bool isMale = random.next(2) == 0;
	const int raceID = random.next(8);
	const int charClassDefID = random.next(charClassLibrary.getDefinitionCount());
	const CharacterClassDefinition &charClassDef = charClassLibrary.getDefinition(charClassDefID);
	const int portraitID = random.next(10);
	const CoordDouble3 position(ChunkInt2::Zero, VoxelDouble3::Zero);
	const Double3 direction(CardinalDirection::North.x, 0.0, CardinalDirection::North.y);
	const Double3 velocity = Double3::Zero;
	const int weaponID = [&random, &charClassDef]()
	{
		// Pick a random weapon available to the player's class.
		std::vector<int> weapons(charClassDef.getAllowedWeaponCount());
		for (int i = 0; i < static_cast<int>(weapons.size()); i++)
		{
			weapons[i] = charClassDef.getAllowedWeapon(i);
		}

		// Add fists.
		weapons.push_back(-1);

		return weapons.at(random.next(static_cast<int>(weapons.size())));
	}();

	return Player(name, isMale, raceID, charClassDefID, portraitID, position, direction, velocity,
		Player::DEFAULT_WALK_SPEED, Player::DEFAULT_RUN_SPEED, weaponID, exeData);
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

double Player::getJumpMagnitude() const
{
	return Player::JUMP_VELOCITY;
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

bool Player::onGround(const WorldData &worldData) const
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
	const NewInt3 feetVoxel(
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
	this->camera.rotate(dx * (100.0 * hSensitivity),
		dy * (100.0 * vSensitivity), pitchLimit);
}

void Player::lookAt(const CoordDouble3 &point)
{
	this->camera.lookAt(point);
}

void Player::handleCollision(const WorldData &worldData, double dt)
{
	const LevelData &activeLevel = worldData.getActiveLevel();

	auto getVoxelDef = [&activeLevel](const NewInt3 &voxel) -> VoxelDefinition
	{
		const VoxelGrid &voxelGrid = activeLevel.getVoxelGrid();

		// Voxels outside the world are air.
		if ((voxel.x < 0) || (voxel.x >= voxelGrid.getWidth()) ||
			(voxel.y < 0) || (voxel.y >= voxelGrid.getHeight()) ||
			(voxel.z < 0) || (voxel.z >= voxelGrid.getDepth()))
		{
			return VoxelDefinition();
		}
		else
		{
			const uint16_t voxelID = voxelGrid.getVoxel(voxel.x, voxel.y, voxel.z);
			return voxelGrid.getVoxelDef(voxelID);
		}
	};

	// Coordinates of the base of the voxel the feet are in.
	// - @todo: add delta velocity Y?
	const int feetVoxelY = static_cast<int>(std::floor(
		this->getFeetY() / activeLevel.getCeilingHeight()));

	// Get the voxel data for each voxel the player would touch on each axis.
	const NewDouble3 absolutePlayerPoint = VoxelUtils::coordToNewPoint(this->getPosition());
	const NewInt3 absolutePlayerVoxel = VoxelUtils::pointToVoxel(absolutePlayerPoint);
	const NewInt3 xVoxel(
		static_cast<SNInt>(std::floor(absolutePlayerPoint.x + (this->velocity.x * dt))),
		feetVoxelY,
		absolutePlayerVoxel.z);
	const NewInt3 yVoxel(
		absolutePlayerVoxel.x,
		feetVoxelY,
		absolutePlayerVoxel.z);
	const NewInt3 zVoxel(
		absolutePlayerVoxel.x,
		feetVoxelY,
		static_cast<WEInt>(std::floor(absolutePlayerPoint.z + (this->velocity.z * dt))));

	const VoxelDefinition &xVoxelDef = getVoxelDef(xVoxel);
	const VoxelDefinition &yVoxelDef = getVoxelDef(yVoxel);
	const VoxelDefinition &zVoxelDef = getVoxelDef(zVoxel);

	// Check horizontal collisions.

	// -- Temp hack until Y collision detection is implemented --
	// - @todo: formalize the collision calculation and get rid of this hack.
	//   We should be able to cover all collision cases in Arena now.
	auto wouldCollideWithVoxel = [&activeLevel](const NewInt3 &voxel, const VoxelDefinition &voxelDef)
	{
		if (voxelDef.type == ArenaTypes::VoxelType::TransparentWall)
		{
			// Transparent wall collision.
			const VoxelDefinition::TransparentWallData &transparent = voxelDef.transparentWall;
			return transparent.collider;
		}
		else if (voxelDef.type == ArenaTypes::VoxelType::Edge)
		{
			// Edge collision.
			// - @todo: treat as edge, not solid voxel.
			const VoxelDefinition::EdgeData &edge = voxelDef.edge;
			return edge.collider;
		}
		else
		{
			// General voxel collision.
			const bool isEmpty = voxelDef.type == ArenaTypes::VoxelType::None;
			const bool isOpenDoor = [&activeLevel, &voxel, &voxelDef]()
			{
				if (voxelDef.type == ArenaTypes::VoxelType::Door)
				{
					const VoxelInstance *doorInst = activeLevel.tryGetVoxelInstance(voxel, VoxelInstance::Type::OpenDoor);
					const bool isClosed = doorInst == nullptr;
					return !isClosed;
				}
				else
				{
					return false;
				}
			}();

			// -- Temporary hack for "on voxel enter" transitions --
			// - @todo: replace with "on would enter voxel" event and near facing check.
			const bool isLevelTransition = [&activeLevel, &voxel, &voxelDef]()
			{
				if (voxelDef.type == ArenaTypes::VoxelType::Wall)
				{
					const LevelData::Transitions &transitions = activeLevel.getTransitions();
					const auto transitionIter = transitions.find(NewInt2(voxel.x, voxel.z));
					if (transitionIter == transitions.end())
					{
						return false;
					}

					return transitionIter->second.getType() != LevelData::Transition::Type::Menu;
				}
				else
				{
					return false;
				}
			}();

			return !isEmpty && !isOpenDoor && !isLevelTransition;
		}
	};

	if (wouldCollideWithVoxel(xVoxel, xVoxelDef))
	{
		this->velocity.x = 0.0;
	}

	if (wouldCollideWithVoxel(zVoxel, zVoxelDef))
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

void Player::setDirectionToHorizon()
{
	const CoordDouble3 &coord = this->getPosition();
	const NewDouble2 groundDirection = this->getGroundDirection();
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

void Player::updatePhysics(const WorldData &worldData, bool collision, double dt)
{
	// Acceleration from gravity (always).
	this->accelerate(-Double3::UnitY, Player::GRAVITY, false, dt);

	// Temp: get floor Y until Y collision is implemented.
	const double floorY = [&worldData]()
	{
		const LevelData &activeLevel = worldData.getActiveLevel();
		return activeLevel.getCeilingHeight();
	}();

	// Change the player's velocity based on collision.
	if (collision)
	{
		this->handleCollision(worldData, dt);

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
	const NewDouble3 absolutePlayerPoint = VoxelUtils::coordToNewPoint(this->getPosition());
	const Double3 newPosition = absolutePlayerPoint + (this->velocity * dt);

	// Update the position if valid.
	if (std::isfinite(newPosition.length()))
	{
		this->camera.position = VoxelUtils::newPointToCoord(newPosition);
	}

	if (this->onGround(worldData))
	{
		// Slow down the player's horizontal velocity with some friction.
		Double2 velocityXZ(this->velocity.x, this->velocity.z);
		Double2 frictionDirection = Double2(-velocityXZ.x, -velocityXZ.y).normalized();
		double frictionMagnitude = velocityXZ.length() * Player::FRICTION;

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
	const WorldData &worldData = game.getGameData().getActiveWorld();
	this->updatePhysics(worldData, game.getOptions().getMisc_Collision(), dt);

	// Tick weapon animation.
	this->weaponAnimation.tick(dt);
}
