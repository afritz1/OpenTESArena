#include "SkyMoonDefinition.h"

#include "components/debug/Debug.h"

void SkyMoonDefinition::init(Buffer<TextureAsset> &&textureAssets)
{
	this->textureAssets = std::move(textureAssets);
}

int SkyMoonDefinition::getTextureCount() const
{
	return static_cast<int>(this->textureAssets.getCount());
}

const TextureAsset &SkyMoonDefinition::getTextureAsset(int index) const
{
	return this->textureAssets.get(index);
}
