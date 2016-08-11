#include "SpriteReference.h"

SpriteReference::SpriteReference(int offset, int count)
{
	this->offset = offset;
	this->count = count;
}

SpriteReference::~SpriteReference()
{

}

int SpriteReference::getOffset() const
{
	return this->offset;
}

int SpriteReference::getRectangleCount() const
{
	return this->count;
}
