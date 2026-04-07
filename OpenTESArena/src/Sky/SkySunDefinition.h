#pragma once

#include "../Assets/TextureAsset.h"

struct SkySunDefinition
{
	TextureAsset textureAsset;

	void init(TextureAsset &&textureAsset);
};
