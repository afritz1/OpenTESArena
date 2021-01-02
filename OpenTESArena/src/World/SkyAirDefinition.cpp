#include "SkyAirDefinition.h"

void SkyAirDefinition::init(TextureBuilderID textureBuilderID)
{
	this->textureBuilderID = textureBuilderID;
}

TextureBuilderID SkyAirDefinition::getTextureBuilderID() const
{
	return this->textureBuilderID;
}
