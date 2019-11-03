#include "EntityData.h"

EntityData::EntityData(int flatIndex, int yOffset, bool collider, bool puddle, bool doubleScale,
	bool dark, bool transparent, bool ceiling)
{
	this->flatIndex = flatIndex;
	this->yOffset = yOffset;
	this->collider = collider;
	this->puddle = puddle;
	this->doubleScale = doubleScale;
	this->dark = dark;
	this->transparent = transparent;
	this->ceiling = ceiling;
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

bool EntityData::isDoubleScale() const
{
	return this->doubleScale;
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

EntityAnimationData &EntityData::getAnimationData()
{
	return this->animationData;
}

const EntityAnimationData &EntityData::getAnimationData() const
{
	return this->animationData;
}
