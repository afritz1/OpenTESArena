#include "SkyObjectDefinition.h"

SkyObjectDefinition::SkyObjectDefinition()
{
	this->animationSeconds = 0.0;
	this->shadingType = static_cast<ShadingType>(-1);
	this->type = static_cast<Type>(-1);
	this->width = 0.0;
	this->height = 0.0;
}

void SkyObjectDefinition::init(Type type, double width, double height, const Double2 &anchor,
	ShadingType shadingType, double animationSeconds, const TextureManager::IdGroup<ImageID> &imageIDs)
{
	this->type = type;
	this->width = width;
	this->height = height;
	this->anchor = anchor;
	this->shadingType = shadingType;
	this->animationSeconds = animationSeconds;
	this->imageIDs = imageIDs;
}

int SkyObjectDefinition::getImageIdCount() const
{
	return this->imageIDs.getCount();
}

ImageID SkyObjectDefinition::getImageID(int index) const
{
	return this->imageIDs.getID(index);
}

bool SkyObjectDefinition::hasAnimation() const
{
	return this->animationSeconds > 0.0;
}

double SkyObjectDefinition::getAnimationSeconds() const
{
	return this->animationSeconds;
}

SkyObjectDefinition::ShadingType SkyObjectDefinition::getShadingType() const
{
	return this->shadingType;
}

SkyObjectDefinition::Type SkyObjectDefinition::getType() const
{
	return this->type;
}

double SkyObjectDefinition::getWidth() const
{
	return this->width;
}

double SkyObjectDefinition::getHeight() const
{
	return this->height;
}
