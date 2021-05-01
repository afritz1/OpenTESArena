#include "SkyMoonDefinition.h"

#include "components/debug/Debug.h"

void SkyMoonDefinition::init(Buffer<TextureAssetReference> &&textureAssetRefs)
{
	this->textureAssetRefs = std::move(textureAssetRefs);
}

int SkyMoonDefinition::getTextureCount() const
{
	return static_cast<int>(this->textureAssetRefs.getCount());
}

const TextureAssetReference &SkyMoonDefinition::getTextureAssetRef(int index) const
{
	return this->textureAssetRefs.get(index);
}
