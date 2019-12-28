#include "EntityData.h"

EntityData::EntityData()
{
	this->flatIndex = -1;
	this->yOffset = 0;
	this->collider = false;
	this->puddle = false;
	this->largeScale = false;
	this->dark = false;
	this->transparent = false;
	this->ceiling = false;
	this->mediumScale = false;
}

void EntityData::init(std::string &&displayName, int flatIndex, int yOffset, bool collider,
	bool puddle, bool largeScale, bool dark, bool transparent, bool ceiling, bool mediumScale)
{
	this->displayName = std::move(displayName);
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

std::string_view EntityData::getDisplayName() const
{
	return this->displayName;
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
