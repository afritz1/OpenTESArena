#include "Sprite.h"

#include "../Entities/Directable.h"
#include "../Utilities/Debug.h"

Sprite::Sprite(const Double3 &point, const Double3 &direction,
	double width, double height)
	: point(point), direction(direction)
{
	this->width = width;
	this->height = height;
}

Sprite::~Sprite()
{

}
