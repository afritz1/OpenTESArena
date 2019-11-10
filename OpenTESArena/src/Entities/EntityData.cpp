#include "EntityData.h"

EntityData::EntityData(int flatIndex, int yOffset, bool collider, bool puddle, bool largeScale,
	bool dark, bool transparent, bool ceiling, bool mediumScale)
{
	this->flatIndex = flatIndex;
	this->yOffset = yOffset;
	this->collider = collider;
	this->puddle = puddle;
	this->largeScale = largeScale;
	this->dark = dark;
	this->transparent = transparent;
	this->ceiling = ceiling;
	this->mediumScale = mediumScale;
}

int EntityData::getFlatIndex() const
{
	return this->flatIndex;
}

int EntityData::getYOffset() const
{
	return this->yOffset;
}

bool EntityData::isCollider() const
{
	return this->collider;
}

bool EntityData::isPuddle() const
{
	return this->puddle;
}

bool EntityData::isLargeScale() const
{
	return this->largeScale;
}

bool EntityData::isDark() const
{
	return this->dark;
}

bool EntityData::isTransparent() const
{
	return this->transparent;
}

bool EntityData::isOnCeiling() const
{
	return this->ceiling;
}

bool EntityData::isMediumScale() const
{
	return this->mediumScale;
}

EntityAnimationData &EntityData::getAnimationData()
{
	return this->animationData;
}

const EntityAnimationData &EntityData::getAnimationData() const
{
	return this->animationData;
}
