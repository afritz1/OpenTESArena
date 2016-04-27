#include <cassert>

#include "AccessoryArtifactData.h"

#include "AccessoryType.h"
#include "ItemType.h"
#include "MetalType.h"

AccessoryArtifactData::AccessoryArtifactData(const std::string &displayName, 
	const std::string &flavorText, const std::vector<ProvinceName> &provinces, 
	const AccessoryType &accessoryType, const MetalType &metalType)
	: ArtifactData(displayName, flavorText, provinces)
{
	this->accessoryType = accessoryType;
	this->metalType = metalType;

	assert(this->accessoryType == accessoryType);
	assert(this->metalType == metalType);
}

AccessoryArtifactData::~AccessoryArtifactData()
{

}

std::unique_ptr<ArtifactData> AccessoryArtifactData::clone() const
{
	return std::unique_ptr<ArtifactData>(new AccessoryArtifactData(
		this->getDisplayName(), this->getFlavorText(), this->getProvinces(),
		this->getAccessoryType(), this->getMetalType()));
}

const AccessoryType &AccessoryArtifactData::getAccessoryType() const
{
	return this->accessoryType;
}

const MetalType &AccessoryArtifactData::getMetalType() const
{
	return this->metalType;
}

ItemType AccessoryArtifactData::getItemType() const
{
	return ItemType::Accessory;
}
