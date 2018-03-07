#include <cassert>

#include "ArmorMaterialType.h"
#include "MediumArmorMaterial.h"

MediumArmorMaterial::MediumArmorMaterial()
{

}

MediumArmorMaterial::~MediumArmorMaterial()
{

}

std::unique_ptr<ArmorMaterial> MediumArmorMaterial::clone() const
{
	return std::make_unique<MediumArmorMaterial>();
}

ArmorMaterialType MediumArmorMaterial::getMaterialType() const
{
	return ArmorMaterialType::Chain;
}

int MediumArmorMaterial::getArmorRating() const
{
	return 6;
}

int MediumArmorMaterial::getConditionMultiplier() const
{
	return 2;
}

double MediumArmorMaterial::getWeightMultiplier() const
{
	return 0.75;
}

bool MediumArmorMaterial::isEnchantable() const
{
	return false;
}

std::string MediumArmorMaterial::toString() const
{
	return "Chain";
}
