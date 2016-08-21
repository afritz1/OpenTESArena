#include <cassert>
#include <map>

#include "CharacterClass.h"

#include "CharacterClassCategory.h"
#include "../Items/ArmorMaterialType.h"
#include "../Items/ShieldType.h"
#include "../Items/WeaponType.h"

CharacterClass::CharacterClass(const std::string &displayName, 
	CharacterClassCategoryName categoryName, bool castsMagic, int32_t startingHealth, 
	int32_t healthDice, const std::vector<ArmorMaterialType> &allowedArmors, 
	const std::vector<ShieldType> &allowedShields, 
	const std::vector<WeaponType> &allowedWeapons)
{
	this->displayName = displayName;
	this->categoryName = categoryName;
	this->castsMagic = castsMagic;
	this->startingHealth = startingHealth;
	this->healthDice = healthDice;
	this->allowedArmors = allowedArmors;
	this->allowedShields = allowedShields;
	this->allowedWeapons = allowedWeapons;
}

CharacterClass::~CharacterClass()
{

}

const std::string &CharacterClass::getDisplayName() const
{
	return this->displayName;
}

CharacterClassCategoryName CharacterClass::getClassCategoryName() const
{
	return this->categoryName;
}

bool CharacterClass::canCastMagic() const
{
	return this->castsMagic;
}

int32_t CharacterClass::getStartingHealth() const
{
	return this->startingHealth;
}

int32_t CharacterClass::getHealthDice() const
{
	return this->healthDice;
}

const std::vector<ArmorMaterialType> &CharacterClass::getAllowedArmors() const
{
	return this->allowedArmors;
}

const std::vector<ShieldType> &CharacterClass::getAllowedShields() const
{
	return this->allowedShields;
}

const std::vector<WeaponType> &CharacterClass::getAllowedWeapons() const
{
	return this->allowedWeapons;
}
