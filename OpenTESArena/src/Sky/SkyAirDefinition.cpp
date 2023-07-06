#include "SkyAirDefinition.h"

void SkyAirDefinition::init(TextureAsset &&textureAsset)
{
	this->textureAsset = std::move(textureAsset);
}
