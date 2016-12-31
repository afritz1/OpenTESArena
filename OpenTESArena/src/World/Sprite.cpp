#include "Sprite.h"

#include "../Entities/Directable.h"
#include "../Utilities/Debug.h"

Sprite::Sprite(const Float3d &point, const Float3d &direction,
	double width, double height)
	: point(point), direction(direction)
{
	this->width = width;
	this->height = height;
}

Sprite::~Sprite()
{

}
