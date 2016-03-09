#include <cassert>

#include "CharacterEquipment.h"
#include "../Items/Accessory.h"
#include "../Items/BodyArmor.h"
#include "../Items/Shield.h"
#include "../Items/Trinket.h"
#include "../Items/Weapon.h"

#include <memory>

// Leave this class for later until containers are programmed. Then, the pointer
// type (either shared_ptr or weak_ptr) can be decided here.

/*
CharacterEquipment::CharacterEquipment()
{
	// Initialize each list to empty.
	this->accessories = std::vector<std::weak_ptr<Accessory>>();
	this->bodyArmors = std::map<BodyPartName, std::weak_ptr<BodyArmor>>();
	this->shield = std::weak_ptr<Shield>();
	this->trinkets = std::vector<std::weak_ptr<Trinket>>();
	this->weapon = std::weak_ptr<Weapon>();

	assert(this->accessories.size() == 0);
	assert(this->bodyArmors.size() == 0);
	assert(this->shield != nullptr);
	assert(this->trinkets.size() == 0);
	assert(this->weapon != nullptr);
}

CharacterEquipment::~CharacterEquipment()
{

}

std::vector<Accessory*> CharacterEquipment::getAccessories(AccessoryType accessoryType) const
{
	auto accessories = std::vector<Accessory*>();

	// It's okay to copy the pointers with unique_ptr::get() because the returned
	// vector's lifetime is shorter than the CharacterEquipment object's lifetime,
	// and all of the pointers are const-qualified, too. unique_ptrs shouldn't keep 
	// data from being modified, they should just monitor the lifetime of the data.
	for (auto &accessory : this->accessories)
	{
		if (accessory->getAccessoryType() == accessoryType)
		{
			accessories.push_back(accessory.get());
		}
	}

	return accessories;
}

BodyArmor *CharacterEquipment::getBodyArmor(BodyPartName partName) const
{
	return this->bodyArmors.at(partName).get();
}

Shield *CharacterEquipment::getShield() const
{
	return this->shield.get();
}

std::vector<Trinket*> CharacterEquipment::getTrinkets(TrinketName trinketName) const
{
	auto trinkets = std::vector<Trinket*>();

	// It's okay to copy the pointers with unique_ptr::get() because the returned
	// vector's lifetime is shorter than the CharacterEquipment object's lifetime,
	// and all of the pointers are const-qualified, too.
	for (auto &trinket : this->trinkets)
	{
		if (trinket->getTrinketName() == trinketName)
		{
			trinkets.push_back(trinket.get());
		}
	}

	return trinkets;
}

Weapon *CharacterEquipment::getWeapon() const
{
	return this->weapon.get();
}
*/