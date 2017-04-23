#include <cassert>
#include <cmath>
#include <vector>

#include "Player.h"

#include "EntityType.h"
#include "GenderName.h"
#include "../Game/Game.h"
#include "../Items/WeaponType.h"
#include "../Math/Constants.h"
#include "../Utilities/String.h"

Player::Player(const std::string &displayName, GenderName gender, int raceID, 
	const CharacterClass &charClass, int portraitID, const Double3 &position, 
	const Double3 &direction, const Double3 &velocity, double maxWalkSpeed, 
	double maxRunSpeed)
	: charClass(charClass), displayName(displayName), camera(position, direction), 
	velocity(velocity), weaponAnimation(WeaponType::Fists /* Placeholder for now. */) 
{
	assert(portraitID >= 0);

	this->maxWalkSpeed = maxWalkSpeed;
	this->maxRunSpeed = maxRunSpeed;
	this->gender = gender;
	this->raceID = raceID;
	this->portraitID = portraitID;
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
	// Multiply sensitivities by 100.0 so the values in options.txt are nicer.
	this->camera.rotate(dx * (100.0 * hSensitivity), dy * (100.0 * vSensitivity));
}

void Player::lookAt(const Double3 &point)
{
	this->camera.lookAt(point);
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
		this->velocity = Double3(0.0f, 0.0f, 0.0f);
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

	// Tick weapon animation.
	this->weaponAnimation.tick(dt);
}
