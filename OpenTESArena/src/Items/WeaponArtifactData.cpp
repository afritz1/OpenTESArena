#include <cassert>

#include "WeaponArtifactData.h"

#include "ItemType.h"
#include "MetalType.h"
#include "WeaponType.h"

WeaponArtifactData::WeaponArtifactData(const std::string &displayName, 
	const std::string &flavorText, const std::vector<ProvinceName> &provinces, 
	WeaponType weaponType, MetalType metalType)
	: ArtifactData(displayName, flavorText, provinces)
{
	this->weaponType = weaponType;
	this->metalType = metalType;
}

WeaponArtifactData::~WeaponArtifactData()
{

}

std::unique_ptr<ArtifactData> WeaponArtifactData::clone() const
{
	return std::unique_ptr<ArtifactData>(new WeaponArtifactData(
		this->getDisplayName(), this->getFlavorText(), this->getProvinces(),
		this->getWeaponType(), this->getMetalType()));
}

WeaponType WeaponArtifactData::getWeaponType() const
{
	return this->weaponType;
}

MetalType WeaponArtifactData::getMetalType() const
{
	return this->metalType;
}

ItemType WeaponArtifactData::getItemType() const
{
	return ItemType::Weapon;
}
