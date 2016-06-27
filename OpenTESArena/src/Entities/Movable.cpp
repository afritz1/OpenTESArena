#include <cassert>
#include <cmath>

#include "Movable.h"

Movable::Movable(const Float3d &velocity, double maxWalkSpeed, double maxRunSpeed)
	: velocity(velocity)
{
	this->maxWalkSpeed = maxWalkSpeed;
	this->maxRunSpeed = maxRunSpeed;
}

Movable::~Movable()
{

}

const Float3d &Movable::getVelocity() const
{
	return this->velocity;
}

double Movable::getMaxWalkSpeed() const
{
	return this->maxWalkSpeed;
}

double Movable::getMaxRunSpeed() const
{
	return this->maxRunSpeed;
}

void Movable::accelerate(const Float3d &direction, double magnitude,
	bool isRunning, double dt)
{
	assert(dt >= 0.0);
	assert(magnitude >= 0.0);
	assert(std::isfinite(magnitude));
	assert(direction.isNormalized());

	// Simple Euler integration for updating velocity.
	Float3d newVelocity = this->velocity + ((direction * magnitude) * dt);
	
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

void Movable::setVelocity(const Float3d &velocity)
{
	assert(std::isfinite(velocity.length()));

	this->velocity = velocity;
}

void Movable::setMaxWalkSpeed(double maxWalkSpeed)
{
	assert(std::isfinite(maxWalkSpeed));

	this->maxWalkSpeed = maxWalkSpeed;
}

void Movable::setMaxRunSpeed(double maxRunSpeed)
{
	assert(std::isfinite(maxRunSpeed));

	this->maxRunSpeed = maxRunSpeed;
}
