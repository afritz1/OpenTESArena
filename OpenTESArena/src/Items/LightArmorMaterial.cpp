#include <cassert>

#include "LightArmorMaterial.h"

#include "ArmorMaterialType.h"

LightArmorMaterial::LightArmorMaterial()
{

}

LightArmorMaterial::~LightArmorMaterial()
{

}

std::unique_ptr<ArmorMaterial> LightArmorMaterial::clone() const
{
	return std::unique_ptr<ArmorMaterial>(new LightArmorMaterial());
}

ArmorMaterialType LightArmorMaterial::getMaterialType() const
{
	return ArmorMaterialType::Leather;
}

int32_t LightArmorMaterial::getArmorRating() const
{
	return 3;
}

int32_t LightArmorMaterial::getConditionMultiplier() const
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
