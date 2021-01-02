#include "SkyMoonDefinition.h"

void SkyMoonDefinition::init(const TextureBuilderIdGroup &textureBuilderIDs)
{
	this->textureBuilderIDs = textureBuilderIDs;
}

int SkyMoonDefinition::getTextureCount() const
{
	return this->textureBuilderIDs.getCount();
}

TextureBuilderID SkyMoonDefinition::getTextureBuilderID(int index) const
{
	return this->textureBuilderIDs.getID(index);
}
