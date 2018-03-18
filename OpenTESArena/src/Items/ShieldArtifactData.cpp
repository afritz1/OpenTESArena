#include <cassert>

#include "ArmorType.h"
#include "MetalType.h"
#include "ShieldArtifactData.h"
#include "ShieldType.h"

ShieldArtifactData::ShieldArtifactData(const std::string &displayName, 
	const std::string &flavorText, const std::vector<int> &provinceIDs, 
	ShieldType shieldType, MetalType metalType)
	: ArmorArtifactData(displayName, flavorText, provinceIDs)
{
	this->shieldType = shieldType;
	this->metalType = metalType;
}

std::unique_ptr<ArtifactData> ShieldArtifactData::clone() const
{
	return std::make_unique<ShieldArtifactData>(
		this->getDisplayName(), this->getFlavorText(), this->getProvinceIDs(),
		this->getShieldType(), this->getMetalType());
}

ShieldType ShieldArtifactData::getShieldType() const
{
	return this->shieldType;
}

MetalType ShieldArtifactData::getMetalType() const
{
	return this->metalType;
}

ArmorType ShieldArtifactData::getArmorType() const
{
	return ArmorType::Shield;
}

