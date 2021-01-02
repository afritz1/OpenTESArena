#include "SkySunDefinition.h"

void SkySunDefinition::init(TextureBuilderID textureBuilderID)
{
	this->textureBuilderID = textureBuilderID;
}

TextureBuilderID SkySunDefinition::getTextureBuilderID() const
{
	return this->textureBuilderID;
}
