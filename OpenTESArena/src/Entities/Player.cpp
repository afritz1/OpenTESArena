#include <cassert>

#include "Player.h"

#include "CoordinateFrame.h"
#include "../Game/GameState.h"
#include "../Math/Constants.h"
#include "../Math/Quaternion.h"

Player::Player(const std::string &displayName, const Float3d &position, 
	const Float3d &direction, const Float3d &velocity, EntityManager &entityManager)
	: Entity(EntityType::Player, position, entityManager), Directable(direction), 
	Movable(velocity)
{
	this->displayName = displayName;
}

Player::Player(const std::string &displayName, const Float3d &position,
	EntityManager &entityManager)
	: Player(displayName, position, Float3d(1.0, 0.0, 0.0), Float3d(), entityManager) { }

Player::~Player()
{

}

std::unique_ptr<Entity> Player::clone(EntityManager &entityManager) const
{
	return std::unique_ptr<Entity>(new Player(
		this->getDisplayName(), this->getPosition(), this->getDirection(),
		this->getVelocity(), entityManager));
}

const std::string &Player::getDisplayName() const
{
	return this->displayName;
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

	double lookRightRads = (hSensitivity * dx) * DEG_TO_RAD;
	double lookUpRads = (vSensitivity * dy) * DEG_TO_RAD;

	if (!std::isfinite(lookRightRads))
	{
		lookRightRads = 0.0;
	}

	if (!std::isfinite(lookUpRads))
	{
		lookUpRads = 0.0;
	}

	const double zoom = 1.0 / std::tan((verticalFOV * 0.5) * DEG_TO_RAD);

	const double currentDec = std::acos(this->getDirection().normalized().getY());
	const double requestedDec = currentDec - lookUpRads;

	const double MIN_UP_TILT_DEG = 0.10;
	const double zenithMaxDec = MIN_UP_TILT_DEG * DEG_TO_RAD;
	const double zenithMinDec = (180.0 - MIN_UP_TILT_DEG) * DEG_TO_RAD;

	lookUpRads = (requestedDec > zenithMinDec) ? (currentDec - zenithMinDec) :
		((requestedDec < zenithMaxDec) ? (currentDec - zenithMaxDec) : lookUpRads);

	this->pitch(lookUpRads / zoom);
	this->yaw(-lookRightRads / zoom);
}

void Player::tick(GameState *gameState, double dt)
{

}
