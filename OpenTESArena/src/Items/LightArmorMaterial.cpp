#include <cassert>

#include "ArmorMaterialType.h"
#include "LightArmorMaterial.h"

LightArmorMaterial::LightArmorMaterial()
{

}

LightArmorMaterial::~LightArmorMaterial()
{

}

std::unique_ptr<ArmorMaterial> LightArmorMaterial::clone() const
{
	return std::make_unique<LightArmorMaterial>();
}

ArmorMaterialType LightArmorMaterial::getMaterialType() const
{
	return ArmorMaterialType::Leather;
}

int LightArmorMaterial::getArmorRating() const
{
	return 3;
}

int LightArmorMaterial::getConditionMultiplier() const
{
	return 1;
}

double LightArmorMaterial::getWeightMultiplier() const
{
	return 0.25;
}

bool LightArmorMaterial::isEnchantable() const
{
	return false;
}

std::string LightArmorMaterial::toString() const
{
	return "Leather";
}
