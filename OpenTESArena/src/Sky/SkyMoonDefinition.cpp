#include "SkyMoonDefinition.h"

void SkyMoonDefinition::init(Buffer<TextureAsset> &&textureAssets)
{
	this->textureAssets = std::move(textureAssets);
}
