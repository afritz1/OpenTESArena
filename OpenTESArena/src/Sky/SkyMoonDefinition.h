#pragma once

#include "../Assets/TextureAsset.h"

#include "components/utilities/Buffer.h"

struct SkyMoonDefinition
{
	// One texture per phase.
	Buffer<TextureAsset> textureAssets;

	void init(Buffer<TextureAsset> &&textureAssets);
};
