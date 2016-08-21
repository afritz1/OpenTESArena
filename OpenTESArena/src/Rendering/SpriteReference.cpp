#include "SpriteReference.h"

SpriteReference::SpriteReference(int32_t offset, int32_t count)
{
	this->offset = offset;
	this->count = count;
}

SpriteReference::~SpriteReference()
{

}

int32_t SpriteReference::getOffset() const
{
	return this->offset;
}

int32_t SpriteReference::getRectangleCount() const
{
	return this->count;
}
