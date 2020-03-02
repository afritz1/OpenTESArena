#include "EntityDefinition.h"

EntityDefinition::EntityDefinition()
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
	this->streetLight = false;
}

void EntityDefinition::init(std::string &&displayName, int flatIndex, int yOffset, bool collider,
	bool puddle, bool largeScale, bool dark, bool transparent, bool ceiling, bool mediumScale,
	bool streetLight, const std::optional<int> &lightIntensity,
	const std::optional<uint8_t> &creatureSoundIndex)
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
	this->streetLight = streetLight;
	this->lightIntensity = lightIntensity;
	this->creatureSoundIndex = creatureSoundIndex;
}

std::string_view EntityDefinition::getDisplayName() const
{
	return this->displayName;
}

int EntityDefinition::getFlatIndex() const
{
	return this->flatIndex;
}

int EntityDefinition::getYOffset() const
{
	return this->yOffset;
}

bool EntityDefinition::isCollider() const
{
	return this->collider;
}

bool EntityDefinition::isPuddle() const
{
	return this->puddle;
}

bool EntityDefinition::isLargeScale() const
{
	return this->largeScale;
}

bool EntityDefinition::isDark() const
{
	return this->dark;
}

bool EntityDefinition::isTransparent() const
{
	return this->transparent;
}

bool EntityDefinition::isOnCeiling() const
{
	return this->ceiling;
}

bool EntityDefinition::isMediumScale() const
{
	return this->mediumScale;
}

bool EntityDefinition::isStreetLight() const
{
	return this->streetLight;
}

const int *EntityDefinition::getLightIntensity() const
{
	return this->lightIntensity.has_value() ? &(*this->lightIntensity) : nullptr;
}

const uint8_t *EntityDefinition::getCreatureSoundIndex() const
{
	return this->creatureSoundIndex.has_value() ? &(*this->creatureSoundIndex) : nullptr;
}

EntityAnimationData &EntityDefinition::getAnimationData()
{
	return this->animationData;
}

const EntityAnimationData &EntityDefinition::getAnimationData() const
{
	return this->animationData;
}
