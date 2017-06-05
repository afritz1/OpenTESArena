#include <cassert>
#include <cmath>
#include <vector>

#include "Player.h"

#include "EntityType.h"
#include "GenderName.h"
#include "../Game/Game.h"
#include "../Game/GameData.h"
#include "../Items/WeaponType.h"
#include "../Math/Constants.h"
#include "../Utilities/String.h"
#include "../World/VoxelData.h"
#include "../World/VoxelGrid.h"

Player::Player(const std::string &displayName, GenderName gender, int raceID,
	const CharacterClass &charClass, int portraitID, const Double3 &position,
	const Double3 &direction, const Double3 &velocity, double maxWalkSpeed,
	double maxRunSpeed)
	: displayName(displayName), gender(gender), raceID(raceID), charClass(charClass),
	portraitID(portraitID), camera(position, direction), velocity(velocity),
	maxWalkSpeed(maxWalkSpeed), maxRunSpeed(maxRunSpeed),
	weaponAnimation(WeaponType::Fists /* Placeholder for now. */)
{
	assert(portraitID >= 0);
}

Player::~Player()
{

}

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

WeaponAnimation &Player::getWeaponAnimation()
{
	return this->weaponAnimation;
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

void Player::handleCollision(const VoxelGrid &voxelGrid, double dt)
{
	// To do: Y collision (player standing height == ...?).
	const Int3 playerVoxel(
		static_cast<int>(std::floor(this->camera.position.x)),
		static_cast<int>(std::floor(this->camera.position.y)),
		static_cast<int>(std::floor(this->camera.position.z)));

	auto getVoxel = [&voxelGrid](int x, int y, int z)
	{
		// Voxels outside the world are air.
		if ((x < 0) || (x >= voxelGrid.getWidth()) ||
			(y < 0) || (y >= voxelGrid.getHeight()) ||
			(z < 0) || (z >= voxelGrid.getDepth()))
		{
			return VoxelData(0);
		}
		else
		{
			return voxelGrid.getVoxelData(voxelGrid.getVoxels()[x +
				(y * voxelGrid.getWidth()) +
				(z * voxelGrid.getWidth() * voxelGrid.getHeight())]);
		}
	};

	const VoxelData &xVoxel = getVoxel(
		static_cast<int>(std::floor(this->camera.position.x + (this->velocity.x * dt))),
		playerVoxel.y,
		playerVoxel.z);
	const VoxelData &zVoxel = getVoxel(
		playerVoxel.x,
		playerVoxel.y,
		static_cast<int>(std::floor(this->camera.position.z + (this->velocity.z * dt))));

	// To do: use an axis-aligned bounding box instead of a point, and take the voxel data
	// Y size and offset into consideration as well. There should be collision detection
	// from the player's feet to the player's head.
	if (!xVoxel.isAir())
	{
		this->velocity.x = 0.0;
	}

	if (!zVoxel.isAir())
	{
		this->velocity.z = 0.0;
	}
}

void Player::accelerate(const Double3 &direction, double magnitude,
	bool isRunning, double dt)
{
	assert(dt >= 0.0);
	assert(magnitude >= 0.0);
	assert(std::isfinite(magnitude));
	assert(direction.isNormalized());

	// Simple Euler integration for updating velocity.
	Double3 newVelocity = this->velocity + ((direction * magnitude) * dt);

	if (std::isfinite(newVelocity.length()))
	{
		this->velocity = newVelocity;
	}

	// Don't let the velocity be greater than the max speed for the current 
	// movement state (i.e., walking/running). This will change once jumping
	// and gravity are implemented.
	double maxSpeed = isRunning ? this->maxRunSpeed : this->maxWalkSpeed;
	if (this->velocity.length() > maxSpeed)
	{
		this->velocity = this->velocity.normalized() * maxSpeed;
	}

	// If the velocity is near zero, set it to zero. This fixes a problem where 
	// the velocity could remain at a tiny magnitude and never reach zero.
	if (this->velocity.length() < 0.001)
	{
		this->velocity = Double3(0.0, 0.0, 0.0);
	}
}

void Player::tick(Game &game, double dt)
{
	assert(dt >= 0.0);

	// To do: acceleration from gravity.

	// Change the player's velocity based on collision.
	this->handleCollision(game.getGameData().getVoxelGrid(), dt);
	
	// Simple Euler integration for updating the player's position.
	Double3 newPosition = this->camera.position + (this->velocity * dt);

	// Update the position if valid.
	if (std::isfinite(newPosition.length()))
	{
		this->camera.position = newPosition;
	}

	// Slow down the player with some imaginary friction (as a force). Once jumping 
	// is implemented, change this to affect the ground direction and Y directions 
	// separately.
	const double friction = 8.0;
	Double3 frictionDirection = -this->velocity.normalized();
	double frictionMagnitude = (this->velocity.length() * 0.5) * friction;

	// Change the velocity if friction is valid.
	if (std::isfinite(frictionDirection.length()) && (frictionMagnitude > EPSILON))
	{
		this->accelerate(frictionDirection, frictionMagnitude, true, dt);
	}

	// To do: clamp player above y = 0 (including standing height).

	// Tick weapon animation.
	this->weaponAnimation.tick(dt);
}
