#include <cassert>
#include <cmath>

#include "Directable.h"

#include "../Math/Constants.h"
#include "../Math/CoordinateFrame.h"

Directable::Directable(const Double3 &direction)
{
	this->setDirection(direction);
}

Directable::~Directable()
{

}

Double3 Directable::getGlobalUp()
{
	return Double3(0.0, 1.0, 0.0);
}

const Double3 &Directable::getDirection() const
{
	return this->direction;
}

Double2 Directable::getGroundDirection() const
{
	return Double2(this->direction.x, this->direction.z).normalized();
}

CoordinateFrame Directable::getFrame() const
{
	Double3 forward = this->direction;
	Double3 right = forward.cross(this->getGlobalUp());
	Double3 up = right.cross(forward);
	return CoordinateFrame(forward, right, up);
}

void Directable::setDirection(const Double3 &direction)
{
	assert(direction.isNormalized());

	// Don't allow the direction to be set too close to the global up. Otherwise,
	// it would break the coordinate frame calculation and ground direction.
	double dirGlobalUpDot = direction.dot(this->getGlobalUp());
	if (std::fabs(dirGlobalUpDot) < (1.0 - EPSILON))
	{
		this->direction = direction;
	}
}
