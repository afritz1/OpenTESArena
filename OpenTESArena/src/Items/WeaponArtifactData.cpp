#include "ItemType.h"
#include "MetalType.h"
#include "WeaponArtifactData.h"

WeaponArtifactData::WeaponArtifactData(const std::string &displayName, 
	const std::string &flavorText, const std::vector<int> &provinceIDs, 
	int weaponID, MetalType metalType)
	: ArtifactData(displayName, flavorText, provinceIDs)
{
	this->weaponID = weaponID;
	this->metalType = metalType;
}

std::unique_ptr<ArtifactData> WeaponArtifactData::clone() const
{
	return std::make_unique<WeaponArtifactData>(
		this->getDisplayName(), this->getFlavorText(), this->getProvinceIDs(),
		this->getWeaponID(), this->getMetalType());
}

int WeaponArtifactData::getWeaponID() const
{
	return this->weaponID;
}

MetalType WeaponArtifactData::getMetalType() const
{
	return this->metalType;
}

ItemType WeaponArtifactData::getItemType() const
{
	return ItemType::Weapon;
}
