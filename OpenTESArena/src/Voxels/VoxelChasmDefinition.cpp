#include <algorithm>

#include "ArenaChasmUtils.h"
#include "VoxelChasmDefinition.h"
#include "../Rendering/ArenaRenderUtils.h"

VoxelChasmSolidColor::VoxelChasmSolidColor()
{
	this->paletteIndex = 0;
}

void VoxelChasmSolidColor::init(uint8_t paletteIndex)
{
	this->paletteIndex = paletteIndex;
}

void VoxelChasmAnimated::init(Buffer<TextureAsset> &&textureAssets)
{
	this->textureAssets = std::move(textureAssets);
}

VoxelChasmDefinition::VoxelChasmDefinition()
{
	this->allowsSwimming = false;
	this->isDamaging = false;
	this->isEmissive = false;
	this->animType = static_cast<VoxelChasmAnimationType>(-1);
}

VoxelChasmDefinition::VoxelChasmDefinition(const VoxelChasmDefinition &other)
{
	this->allowsSwimming = other.allowsSwimming;
	this->isDamaging = other.isDamaging;
	this->isEmissive = other.isEmissive;
	this->wallTextureAsset = other.wallTextureAsset;
	this->animType = other.animType;

	if (this->animType == VoxelChasmAnimationType::SolidColor)
	{
		this->solidColor.init(other.solidColor.paletteIndex);
	}
	else if (this->animType == VoxelChasmAnimationType::Animated)
	{
		const Buffer<TextureAsset> &otherTextureAssets = other.animated.textureAssets;
		Buffer<TextureAsset> textureAssets(otherTextureAssets.getCount());
		std::copy(otherTextureAssets.begin(), otherTextureAssets.end(), textureAssets.begin());
		this->animated.init(std::move(textureAssets));
	}
	else
	{
		DebugNotImplementedMsg(std::to_string(static_cast<int>(this->animType)));
	}
}

void VoxelChasmDefinition::initClassic(ArenaTypes::ChasmType chasmType, const TextureAsset &wallTextureAsset,
	TextureManager &textureManager)
{
	this->allowsSwimming = ArenaChasmUtils::allowsSwimming(chasmType);
	this->isDamaging = ArenaChasmUtils::isDamaging(chasmType);
	this->isEmissive = this->isDamaging;
	this->wallTextureAsset = wallTextureAsset;

	if (chasmType == ArenaTypes::ChasmType::Dry)
	{
		this->animType = VoxelChasmAnimationType::SolidColor;
		this->solidColor.init(ArenaRenderUtils::PALETTE_INDEX_DRY_CHASM_COLOR);
	}
	else if ((chasmType == ArenaTypes::ChasmType::Wet) || (chasmType == ArenaTypes::ChasmType::Lava))
	{
		this->animType = VoxelChasmAnimationType::Animated;
		this->animated.init(ArenaChasmUtils::getTextureAssets(chasmType, textureManager));
	}
	else
	{
		DebugNotImplementedMsg(std::to_string(static_cast<int>(chasmType)));
	}
}
