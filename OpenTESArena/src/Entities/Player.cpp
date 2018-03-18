#include <cassert>
#include <cmath>
#include <vector>

#include "EntityType.h"
#include "GenderName.h"
#include "Player.h"
#include "../Game/Game.h"
#include "../Game/GameData.h"
#include "../Game/Options.h"
#include "../Math/Constants.h"
#include "../Math/Random.h"
#include "../Utilities/String.h"
#include "../World/VoxelData.h"
#include "../World/VoxelDataType.h"
#include "../World/VoxelGrid.h"
#include "../World/WorldData.h"

const double Player::HEIGHT = 0.70;
const double Player::DEFAULT_WALK_SPEED = 2.0;
const double Player::DEFAULT_RUN_SPEED = 8.0;
const double Player::STEPPING_HEIGHT = 0.25;
const double Player::JUMP_VELOCITY = 3.0;
const double Player::GRAVITY = 9.81;
const double Player::FRICTION = 4.0;

Player::Player(const std::string &displayName, GenderName gender, int raceID,
	const CharacterClass &charClass, int portraitID, const Double3 &position,
	const Double3 &direction, const Double3 &velocity, double maxWalkSpeed,
	double maxRunSpeed, int weaponID, const ExeData &exeData)
	: displayName(displayName), gender(gender), raceID(raceID), charClass(charClass),
	portraitID(portraitID), camera(position, direction), velocity(velocity),
	maxWalkSpeed(maxWalkSpeed), maxRunSpeed(maxRunSpeed), weaponAnimation(weaponID, exeData) { }

const Double3 &Player::getPosition() const
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

GenderName Player::getGenderName() const
{
	return this->gender;
}

int Player::getRaceID() const
{
	return this->raceID;
}

const CharacterClass &Player::getCharacterClass() const
{
	return this->charClass;
}

Player Player::makeRandom(const std::vector<CharacterClass> &charClasses, const ExeData &exeData)
{
	Random random;
	const std::string name("Player");
	const GenderName gender = (random.next(2) == 0) ? GenderName::Male : GenderName::Female;
	const int raceID = random.next(8);
	const CharacterClass &charClass = charClasses.at(
		random.next(static_cast<int>(charClasses.size())));
	const int portraitID = random.next(10);
	const Double3 position = Double3::Zero;
	const Double3 direction = Double3::UnitX;
	const Double3 velocity = Double3::Zero;
	const int weaponID = [&random, &charClass]()
	{
		// Pick a random weapon available to the player's class.
		std::vector<int> weapons = charClass.getAllowedWeapons();

		// Add fists.
		weapons.push_back(-1);

		return weapons.at(random.next(static_cast<int>(weapons.size())));
	}();

	return Player(name, gender, raceID, charClass, portraitID, position, direction, velocity,
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

Int3 Player::getVoxelPosition() const
{
	return Int3(
		static_cast<int>(std::floor(this->camera.position.x)),
		static_cast<int>(std::floor(this->camera.position.y)),
		static_cast<int>(std::floor(this->camera.position.z)));
}

WeaponAnimation &Player::getWeaponAnimation()
{
	return this->weaponAnimation;
}

double Player::getFeetY() const
{
	return this->camera.position.y - Player::HEIGHT;
}

bool Player::onGround(const WorldData &worldData) const
{
	// To do: find a non-hack way to do this.

	return true;

	// This function seems kind of like a hack right now, since the player's feet
	// will frequently be at Y == 1.0, which is one voxel above the ground, and
	// it won't be considered as "on ground" unless it checks the voxel underneath
	// of this particular Y position (due to the rounding rules being used).
	/*const double feetY = this->getFeetY();
	const double feetVoxelYPos = std::floor(feetY);
	const bool closeEnoughToLowerVoxel = std::abs(feetY - feetVoxelYPos) < EPSILON;
	const Int3 feetVoxel(
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

void Player::teleport(const Double3 &position)
{
	this->camera.position = position;
}

void Player::rotate(double dx, double dy, double hSensitivity, double vSensitivity)
{
	// Multiply sensitivities by 100 so the values in options.txt are nicer.
	this->camera.rotate(dx * (100.0 * hSensitivity), dy * (100.0 * vSensitivity));
}

void Player::lookAt(const Double3 &point)
{
	this->camera.lookAt(point);
}

void Player::handleCollision(const WorldData &worldData, double dt)
{
	auto getVoxel = [&worldData](int x, int y, int z) -> VoxelData
	{
		const auto &level = worldData.getLevels().at(worldData.getCurrentLevel());
		const VoxelGrid &voxelGrid = level.getVoxelGrid();

		// Voxels outside the world are air.
		if ((x < 0) || (x >= voxelGrid.getWidth()) ||
			(y < 0) || (y >= voxelGrid.getHeight()) ||
			(z < 0) || (z >= voxelGrid.getDepth()))
		{
			return VoxelData();
		}
		else
		{
			return voxelGrid.getVoxelData(voxelGrid.getVoxels()[x +
				(y * voxelGrid.getWidth()) +
				(z * voxelGrid.getWidth() * voxelGrid.getHeight())]);
		}
	};

	const double feetY = this->getFeetY();

	// Coordinates of the base of the voxel the feet are in.
	const double feetVoxelYPos = std::floor(feetY + (this->velocity.y * dt));

	// Get the voxel data for each voxel the player would touch on each axis.
	const Int3 playerVoxel = this->getVoxelPosition();
	const VoxelData &xVoxel = getVoxel(
		static_cast<int>(std::floor(this->camera.position.x + (this->velocity.x * dt))),
		playerVoxel.y,
		playerVoxel.z);
	const VoxelData &yVoxel = getVoxel(
		playerVoxel.x,
		static_cast<int>(feetVoxelYPos),
		playerVoxel.z);
	const VoxelData &zVoxel = getVoxel(
		playerVoxel.x,
		playerVoxel.y,
		static_cast<int>(std::floor(this->camera.position.z + (this->velocity.z * dt))));

	// Check horizontal collisions.

	// -- Temp hack until Y collision detection is implemented --
	// - To do: formalize the collision calculation and get rid of this hack.
	//   We should be able to cover all collision cases in Arena now.
	auto wouldCollideWithVoxel = [](const VoxelData &voxelData)
	{
		if (voxelData.dataType == VoxelDataType::TransparentWall)
		{
			// Transparent wall collision.
			const VoxelData::TransparentWallData &transparent = voxelData.transparentWall;
			return transparent.collider;
		}
		else if (voxelData.dataType == VoxelDataType::Edge)
		{
			// Edge collision.
			// - To do: treat as edge, not solid voxel.
			const VoxelData::EdgeData &edge = voxelData.edge;
			return edge.collider;
		}
		else
		{
			// General voxel collision.
			const bool isEmpty = voxelData.dataType == VoxelDataType::None;
			const bool isDoor = voxelData.dataType == VoxelDataType::Door;

			// -- Temporary hack for "on voxel enter" transitions --
			// - To do: replace with "on would enter voxel" event and near facing check.			
			const bool isLevelUpDown = [&voxelData]()
			{
				if (voxelData.dataType == VoxelDataType::Wall)
				{
					const VoxelData::WallData::Type wallType = voxelData.wall.type;
					return (wallType == VoxelData::WallData::Type::LevelUp) ||
						(wallType == VoxelData::WallData::Type::LevelDown);
				}
				else
				{
					return false;
				}
			}();

			return !isEmpty && !isDoor && !isLevelUpDown;
		}
	};

	if (wouldCollideWithVoxel(xVoxel))
	{
		this->velocity.x = 0.0;
	}

	if (wouldCollideWithVoxel(zVoxel))
	{
		this->velocity.z = 0.0;
	}

	this->velocity.y = 0.0;
	// -- end hack --

	// To do: use an axis-aligned bounding box or cylinder instead of a point?
}

void Player::setVelocityToZero()
{
	this->velocity = Double3::Zero;
}

void Player::accelerate(const Double3 &direction, double magnitude,
	bool isRunning, double dt)
{
	assert(dt >= 0.0);
	assert(magnitude >= 0.0);
	assert(std::isfinite(magnitude));
	assert(direction.isNormalized());

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
	assert(direction.isNormalized());
	
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

	// Change the player's velocity based on collision.
	if (collision)
	{
		this->handleCollision(worldData, dt);
	}
	else
	{
		// Keep the player's Y position constant, but otherwise let them act as a ghost.
		this->camera.position.y = Player::HEIGHT + 1.0;
		this->velocity.y = 0.0;
	}

	// Simple Euler integration for updating the player's position.
	Double3 newPosition = this->camera.position + (this->velocity * dt);

	// Update the position if valid.
	if (std::isfinite(newPosition.length()))
	{
		this->camera.position = newPosition;
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
	this->updatePhysics(game.getGameData().getWorldData(), 
		game.getOptions().getCollision(), dt);

	// Tick weapon animation.
	this->weaponAnimation.tick(dt);
}
