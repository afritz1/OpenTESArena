#include "SkySunDefinition.h"

void SkySunDefinition::init(TextureAsset &&textureAsset)
{
	this->textureAsset = std::move(textureAsset);
}
