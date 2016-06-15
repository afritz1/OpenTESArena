#include <cassert>

#include "ShieldArtifactData.h"

#include "ArmorType.h"
#include "MetalType.h"
#include "ShieldType.h"

ShieldArtifactData::ShieldArtifactData(const std::string &displayName, 
	const std::string &flavorText, const std::vector<ProvinceName> &provinces, 
	ShieldType shieldType, MetalType metalType)
	: ArmorArtifactData(displayName, flavorText, provinces)
{
	this->shieldType = shieldType;
	this->metalType = metalType;
}

ShieldArtifactData::~ShieldArtifactData()
{

}

std::unique_ptr<ArtifactData> ShieldArtifactData::clone() const
{
	return std::unique_ptr<ArtifactData>(new ShieldArtifactData(
		this->getDisplayName(), this->getFlavorText(), this->getProvinces(),
		this->getShieldType(), this->getMetalType()));
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

