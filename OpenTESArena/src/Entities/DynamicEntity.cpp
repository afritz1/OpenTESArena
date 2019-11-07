#include <cmath>

#include "DynamicEntity.h"
#include "EntityManager.h"
#include "EntityType.h"
#include "../Game/Game.h"
#include "../Math/Constants.h"
#include "../Math/Quaternion.h"

DynamicEntity::DynamicEntity()
	: direction(Double2::Zero), velocity(Double2::Zero)
{
	this->derivedType = static_cast<DynamicEntityType>(-1);
}

EntityType DynamicEntity::getEntityType() const
{
	return EntityType::Dynamic;
}

DynamicEntityType DynamicEntity::getDerivedType() const
{
	return this->derivedType;
}

const Double2 &DynamicEntity::getDirection() const
{
	return this->direction;
}

const Double2 &DynamicEntity::getVelocity() const
{
	return this->velocity;
}

const Double2 *DynamicEntity::getDestination() const
{
	return this->destination.has_value() ? &this->destination.value() : nullptr;
}

void DynamicEntity::setDerivedType(DynamicEntityType derivedType)
{
	this->derivedType = derivedType;
}

void DynamicEntity::setDirection(const Double2 &direction)
{
	DebugAssert(std::isfinite(direction.lengthSquared()));
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
	this->direction = Double2(q.x, q.z).normalized();
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

void DynamicEntity::lookAt(const Double2 &point)
{
	const Double2 newDirection = (point - this->position).normalized();

	// Only accept the change if it's valid.
	if (std::isfinite(newDirection.lengthSquared()))
	{
		this->direction = newDirection;
	}
}

void DynamicEntity::setDestination(const Double2 *point, double minDistance)
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

void DynamicEntity::setDestination(const Double2 *point)
{
	constexpr double minDistance = Constants::Epsilon;
	this->setDestination(point, minDistance);
}

void DynamicEntity::updatePhysics(const WorldData &worldData, double dt)
{
	// @todo
}

void DynamicEntity::reset()
{
	Entity::reset();
	this->derivedType = static_cast<DynamicEntityType>(-1);
	this->destination = std::nullopt;
}

void DynamicEntity::tick(Game &game, double dt)
{
	Entity::tick(game, dt);

	// Update physics/pathfinding/etc..
	const auto &worldData = game.getGameData().getWorldData();
	this->updatePhysics(worldData, dt);
}
