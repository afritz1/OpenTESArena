#include <cassert>

#include "MediumArmorMaterial.h"

#include "ArmorMaterialType.h"

MediumArmorMaterial::MediumArmorMaterial()
{

}

MediumArmorMaterial::~MediumArmorMaterial()
{

}

std::unique_ptr<ArmorMaterial> MediumArmorMaterial::clone() const
{
	return std::unique_ptr<ArmorMaterial>(new MediumArmorMaterial());
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

std::string MediumArmorMaterial::toString() const
{
	return "Chain";
}
