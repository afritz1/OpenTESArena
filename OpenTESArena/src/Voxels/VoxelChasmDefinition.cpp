#include <algorithm>

#include "ArenaChasmUtils.h"
#include "VoxelChasmDefinition.h"
#include "../Rendering/ArenaRenderUtils.h"

VoxelChasmSolidColor::VoxelChasmSolidColor()
{
	this->paletteIndex = 0;
}

bool VoxelChasmSolidColor::operator==(const VoxelChasmSolidColor &other) const
{
	if (this == &other)
	{
		return true;
	}

	if (this->paletteIndex != other.paletteIndex)
	{
		return false;
	}

	return true;
}

void VoxelChasmSolidColor::init(uint8_t paletteIndex)
{
	this->paletteIndex = paletteIndex;
}

bool VoxelChasmAnimated::operator==(const VoxelChasmAnimated &other) const
{
	if (this == &other)
	{
		return true;
	}

	if (this->textureAssets.getCount() != other.textureAssets.getCount())
	{
		return false;
	}

	for (int i = 0; i < this->textureAssets.getCount(); i++)
	{
		if (this->textureAssets[i] != other.textureAssets[i])
		{
			return false;
		}
	}

	return true;
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

bool VoxelChasmDefinition::operator==(const VoxelChasmDefinition &other) const
{
	if (this == &other)
	{
		return true;
	}

	if (this->allowsSwimming != other.allowsSwimming)
	{
		return false;
	}

	if (this->isDamaging != other.isDamaging)
	{
		return false;
	}

	if (this->isEmissive != other.isEmissive)
	{
		return false;
	}

	if (this->wallTextureAsset != other.wallTextureAsset)
	{
		return false;
	}

	if (this->animType != other.animType)
	{
		return false;
	}

	switch (this->animType)
	{
	case VoxelChasmAnimationType::SolidColor:
		return this->solidColor == other.solidColor;
	case VoxelChasmAnimationType::Animated:
		return this->animated == other.animated;
	default:
		DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(this->animType)));
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
