#include "Directable.h"

Directable::Directable(const Float3d &direction)
{
	this->direction = direction;
}

Directable::~Directable()
{

}

const Float3d &Directable::getDirection() const
{
	return this->direction;
}

void Directable::setDirection(const Float3d &direction)
{
	this->direction = direction;
}
