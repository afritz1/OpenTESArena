#include <cassert>
#include <map>

#include "CharacterClass.h"

#include "CharacterClassCategory.h"
#include "../Items/ArmorMaterialType.h"
#include "../Items/ShieldType.h"
#include "../Items/WeaponType.h"

CharacterClass::CharacterClass(const std::string &displayName, 
	CharacterClassCategoryName categoryName, bool castsMagic, int startingHealth, 
	int healthDice, const std::vector<ArmorMaterialType> &allowedArmors, 
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

	assert(this->displayName == displayName);
	assert(this->categoryName == categoryName);
	assert(this->castsMagic == castsMagic);
	assert(this->startingHealth == startingHealth);
	assert(this->healthDice == healthDice);
	assert(this->allowedArmors == allowedArmors);
	assert(this->allowedShields == allowedShields);
	assert(this->allowedWeapons == allowedWeapons);
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

int CharacterClass::getStartingHealth() const
{
	return this->startingHealth;
}

int CharacterClass::getHealthDice() const
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
