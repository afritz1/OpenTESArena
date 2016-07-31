#include <cassert>
#include <cmath>
#include <vector>

#include "Player.h"

#include "CharacterClass.h"
#include "CharacterGenderName.h"
#include "CharacterRaceName.h"
#include "EntityType.h"
#include "../Game/GameState.h"
#include "../Math/Constants.h"
#include "../Math/CoordinateFrame.h"
#include "../Math/Quaternion.h"
#include "../Utilities/String.h"

Player::Player(const std::string &displayName, CharacterGenderName gender,
	CharacterRaceName raceName, const CharacterClass &charClass, int portraitID,
	const Float3d &position, const Float3d &direction, const Float3d &velocity,
	double maxWalkSpeed, double maxRunSpeed, EntityManager &entityManager)
	: Entity(EntityType::Player, position, entityManager), Directable(direction),
	Movable(velocity, maxWalkSpeed, maxRunSpeed)
{
	assert(portraitID >= 0);

	this->displayName = displayName;
	this->gender = gender;
	this->raceName = raceName;
	this->charClass = std::unique_ptr<CharacterClass>(new CharacterClass(charClass));
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
		this->getDirection(), this->getVelocity(), this->getMaxWalkSpeed(),
		this->getMaxRunSpeed(), entityManager));
}

EntityType Player::getEntityType() const
{
	return EntityType::Player;
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

CharacterGenderName Player::getGenderName() const
{
	return this->gender;
}

CharacterRaceName Player::getRaceName() const
{
	return this->raceName;
}

const CharacterClass &Player::getCharacterClass() const
{
	return *this->charClass.get();
}

void Player::pitch(double radians)
{
	auto frame = this->getFrame();
	auto q = Quaternion::fromAxisAngle(frame.getRight(), radians) *
		Quaternion(this->getDirection(), 0.0);

	this->setDirection(q.getXYZ().normalized());
}

void Player::yaw(double radians)
{
	auto q = Quaternion::fromAxisAngle(Directable::getGlobalUp(), radians) *
		Quaternion(this->getDirection(), 0.0);

	this->setDirection(q.getXYZ().normalized());
}

void Player::rotate(double dx, double dy, double hSensitivity, double vSensitivity,
	double verticalFOV)
{
	assert(std::isfinite(dx));
	assert(std::isfinite(dy));
	assert(std::isfinite(this->getDirection().length()));
	assert(hSensitivity > 0.0);
	assert(vSensitivity > 0.0);
	assert(verticalFOV > 0.0);
	assert(verticalFOV < 180.0);

	// Multiply sensitivities by 100.0 so the values in options.txt are nicer.
	double lookRightRads = (hSensitivity * 100.0 * dx) * DEG_TO_RAD;
	double lookUpRads = (vSensitivity * 100.0 * dy) * DEG_TO_RAD;

	if (!std::isfinite(lookRightRads))
	{
		lookRightRads = 0.0;
	}

	if (!std::isfinite(lookUpRads))
	{
		lookUpRads = 0.0;
	}

	const double currentDec = std::acos(this->getDirection().normalized().getY());
	const double requestedDec = currentDec - lookUpRads;

	const double MIN_UP_TILT_DEG = 0.10;
	const double zenithMaxDec = MIN_UP_TILT_DEG * DEG_TO_RAD;
	const double zenithMinDec = (180.0 - MIN_UP_TILT_DEG) * DEG_TO_RAD;

	lookUpRads = (requestedDec > zenithMinDec) ? (currentDec - zenithMinDec) :
		((requestedDec < zenithMaxDec) ? (currentDec - zenithMaxDec) : lookUpRads);

	// Only divide by zoom when sensitivity depends on field of view (which it doesn't here).
	//const double zoom = 1.0 / std::tan((verticalFOV * 0.5) * DEG_TO_RAD);
	this->pitch(lookUpRads/* / zoom*/);
	this->yaw(-lookRightRads/* / zoom*/);
}

void Player::tick(GameState *gameState, double dt)
{
	assert(dt >= 0.0);
	
	// Simple Euler integration for updating the player's position.
	Float3d newPosition = this->position + (this->getVelocity() * dt);

	// Update the position if valid.
	if (std::isfinite(newPosition.length()))
	{
		this->position = newPosition;
	}

	// Slow down the player with some imaginary friction (as a force). Once jumping 
	// is implemented, change this to affect the ground direction and Y directions 
	// separately.
	double friction = 8.0;
	Float3d frictionDirection = -this->getVelocity().normalized();
	double frictionMagnitude = (this->getVelocity().length() * 0.5) * friction;

	// Change the velocity if friction is valid.
	if (std::isfinite(frictionDirection.length()) && (frictionMagnitude > EPSILON))
	{
		this->accelerate(frictionDirection, frictionMagnitude, true, dt);
	}
}
