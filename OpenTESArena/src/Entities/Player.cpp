#include <cassert>
#include <cmath>
#include <vector>

#include "Player.h"

#include "CharacterRaceName.h"
#include "EntityType.h"
#include "GenderName.h"
#include "../Game/Game.h"
#include "../Math/Constants.h"
#include "../Math/Quaternion.h"
#include "../Utilities/String.h"

Player::Player(const std::string &displayName, GenderName gender,
	CharacterRaceName raceName, const CharacterClass &charClass, int portraitID,
	const Double3 &position, const Double3 &direction, const Double3 &velocity,
	double maxWalkSpeed, double maxRunSpeed, EntityManager &entityManager)
	: Entity(EntityType::Player, entityManager), charClass(charClass),
	displayName(displayName), camera(position, direction), velocity(velocity)
{
	assert(portraitID >= 0);

	this->maxWalkSpeed = maxWalkSpeed;
	this->maxRunSpeed = maxRunSpeed;
	this->gender = gender;
	this->raceName = raceName;
	this->portraitID = portraitID;
}

Player::~Player()
{

}

std::unique_ptr<Entity> Player::clone(EntityManager &entityManager) const
{
	return std::unique_ptr<Entity>(new Player(
		this->getDisplayName(), this->getGenderName(), this->getRaceName(),
		this->getCharacterClass(), this->getPortraitID(), this->getPosition(),
		this->camera.getDirection(), this->velocity, this->maxWalkSpeed,
		this->maxRunSpeed, entityManager));
}

EntityType Player::getEntityType() const
{
	return EntityType::Player;
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

CharacterRaceName Player::getRaceName() const
{
	return this->raceName;
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

void Player::teleport(const Double3 &position)
{
	this->camera.position = position;
}

void Player::rotate(double dx, double dy, double hSensitivity, double vSensitivity)
{
	// Multiply sensitivities by 100.0 so the values in options.txt are nicer.
	this->camera.rotate(dx * (100.0 * hSensitivity), dy * (100.0 * vSensitivity));
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
}

void Player::tick(Game &game, double dt)
{
	assert(dt >= 0.0);

	Double3 &position = this->camera.position;
	
	// Simple Euler integration for updating the player's position.
	Double3 newPosition = position + (this->velocity * dt);

	// Update the position if valid.
	if (std::isfinite(newPosition.length()))
	{
		position = newPosition;
	}

	// Slow down the player with some imaginary friction (as a force). Once jumping 
	// is implemented, change this to affect the ground direction and Y directions 
	// separately.
	double friction = 8.0;
	Double3 frictionDirection = -this->velocity.normalized();
	double frictionMagnitude = (this->velocity.length() * 0.5) * friction;

	// Change the velocity if friction is valid.
	if (std::isfinite(frictionDirection.length()) && (frictionMagnitude > EPSILON))
	{
		this->accelerate(frictionDirection, frictionMagnitude, true, dt);
	}
}
