#include <cassert>

#include "BodyArmorArtifactData.h"

#include "ArmorMaterial.h"
#include "ArmorType.h"
#include "../Entities/BodyPart.h"
#include "../Entities/BodyPartName.h"

BodyArmorArtifactData::BodyArmorArtifactData(const std::string &displayName, 
	const std::string &flavorText, const std::vector<ProvinceName> &provinces, 
	const ArmorMaterial *armorMaterial, const BodyPartName &partName)
	: ArmorArtifactData(displayName, flavorText, provinces)
{
	this->armorMaterial = armorMaterial->clone();
	this->partName = partName;

	assert(this->armorMaterial.get() != nullptr);
	assert(this->partName == partName);
}

BodyArmorArtifactData::~BodyArmorArtifactData()
{

}

std::unique_ptr<ArtifactData> BodyArmorArtifactData::clone() const
{
	return std::unique_ptr<ArtifactData>(new BodyArmorArtifactData(
		this->getDisplayName(), this->getFlavorText(), this->getProvinces(),
		this->getArmorMaterial(), this->getBodyPartName()));
}

const BodyPartName &BodyArmorArtifactData::getBodyPartName() const
{
	return this->partName;
}

const ArmorMaterial *BodyArmorArtifactData::getArmorMaterial() const
{
	return this->armorMaterial.get();
}

ArmorType BodyArmorArtifactData::getArmorType() const
{
	auto armorType = BodyPart(this->getBodyPartName()).getArmorType();
	return armorType;
}
