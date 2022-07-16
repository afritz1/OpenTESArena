#include "VoxelTextureDefinition.h"

#include "components/debug/Debug.h"

VoxelTextureDefinition::VoxelTextureDefinition()
{
	this->textureCount = 0;
}

const TextureAsset &VoxelTextureDefinition::getTextureAsset(int index) const
{
	DebugAssertIndex(this->textureAssets, index);
	DebugAssert(index < this->textureCount);
	return this->textureAssets[index];
}

void VoxelTextureDefinition::addTextureAsset(TextureAsset &&textureAsset)
{
	DebugAssertIndex(this->textureAssets, this->textureCount);
	this->textureAssets[this->textureCount] = std::move(textureAsset);
	this->textureCount++;
}
