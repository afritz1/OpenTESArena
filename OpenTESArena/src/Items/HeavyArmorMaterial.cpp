#include <cassert>

#include "ArmorMaterialType.h"
#include "HeavyArmorMaterial.h"
#include "Metal.h"

HeavyArmorMaterial::HeavyArmorMaterial(MetalType metalType)
	: Metallic(metalType)
{
	
}

HeavyArmorMaterial::~HeavyArmorMaterial()
{

}

std::unique_ptr<ArmorMaterial> HeavyArmorMaterial::clone() const
{
	return std::unique_ptr<ArmorMaterial>(new HeavyArmorMaterial(
		this->getMetal().getMetalType()));
}

ArmorMaterialType HeavyArmorMaterial::getMaterialType() const
{
	return ArmorMaterialType::Plate;
}

int HeavyArmorMaterial::getArmorRating() const
{
	return 9 + this->getMetal().getRatingModifier();
}

int HeavyArmorMaterial::getConditionMultiplier() const
{
	// Add the 3 to compensate for the metal condition multiplier only being 
	// relative to weapons. Now it is relative to armor.
	return this->getMetal().getConditionMultiplier() + 3;
}

double HeavyArmorMaterial::getWeightMultiplier() const
{
	return this->getMetal().getWeightMultiplier();
}

bool HeavyArmorMaterial::isEnchantable() const
{
	return true;
}

std::string HeavyArmorMaterial::toString() const
{
	return this->getMetal().toString();
}
