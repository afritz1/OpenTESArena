#include <cassert>
#include <cmath>

#include "Directable.h"

#include "CoordinateFrame.h"
#include "../Math/Constants.h"

Directable::Directable(const Float3d &direction)
{
	this->setDirection(direction);
}

Directable::~Directable()
{

}

Float3d Directable::getGlobalUp()
{
	return Float3d(0.0, 1.0, 0.0);
}

const Float3d &Directable::getDirection() const
{
	return this->direction;
}

Float2d Directable::getGroundDirection() const
{
	return Float2d(this->direction.getX(), this->direction.getZ()).normalized();
}

CoordinateFrame Directable::getFrame() const
{
	Float3d forward = this->direction;
	Float3d right = forward.cross(this->getGlobalUp());
	Float3d up = right.cross(forward);
	return CoordinateFrame(forward, right, up);
}

void Directable::setDirection(const Float3d &direction)
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
