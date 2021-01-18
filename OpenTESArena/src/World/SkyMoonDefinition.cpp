#include "SkyMoonDefinition.h"

#include "components/debug/Debug.h"

void SkyMoonDefinition::init(std::vector<TextureAssetReference> &&textureAssetRefs)
{
	this->textureAssetRefs = std::move(textureAssetRefs);
}

int SkyMoonDefinition::getTextureCount() const
{
	return static_cast<int>(this->textureAssetRefs.size());
}

const TextureAssetReference &SkyMoonDefinition::getTextureAssetRef(int index) const
{
	DebugAssertIndex(this->textureAssetRefs, index);
	return this->textureAssetRefs[index];
}
