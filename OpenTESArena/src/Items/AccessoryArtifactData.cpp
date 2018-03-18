#include <cassert>

#include "AccessoryArtifactData.h"
#include "AccessoryType.h"
#include "ItemType.h"
#include "MetalType.h"

AccessoryArtifactData::AccessoryArtifactData(const std::string &displayName, 
	const std::string &flavorText, const std::vector<int> &provinceIDs, 
	AccessoryType accessoryType, MetalType metalType)
	: ArtifactData(displayName, flavorText, provinceIDs)
{
	this->accessoryType = accessoryType;
	this->metalType = metalType;
}

std::unique_ptr<ArtifactData> AccessoryArtifactData::clone() const
{
	return std::make_unique<AccessoryArtifactData>(
		this->getDisplayName(), this->getFlavorText(), this->getProvinceIDs(),
		this->getAccessoryType(), this->getMetalType());
}

AccessoryType AccessoryArtifactData::getAccessoryType() const
{
	return this->accessoryType;
}

MetalType AccessoryArtifactData::getMetalType() const
{
	return this->metalType;
}

ItemType AccessoryArtifactData::getItemType() const
{
	return ItemType::Accessory;
}
