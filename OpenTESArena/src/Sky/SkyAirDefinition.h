#pragma once

#include "../Assets/TextureAsset.h"

struct SkyAirDefinition
{
	TextureAsset textureAsset;

	void init(TextureAsset &&textureAsset);
};
