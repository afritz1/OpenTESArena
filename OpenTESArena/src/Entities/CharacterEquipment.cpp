#include "CharacterEquipment.h"
#include "../Items/Accessory.h"
#include "../Items/BodyArmor.h"
#include "../Items/Shield.h"
#include "../Items/Trinket.h"
#include "../Items/Weapon.h"

#include "components/debug/Debug.h"

CharacterEquipment::CharacterEquipment()
{
	// Initialize everything to empty.
	this->accessories = std::vector<Accessory*>();
	this->bodyArmors = std::unordered_map<BodyPartName, BodyArmor*>();
	this->shield = nullptr;
	this->trinkets = std::vector<Trinket*>();
	this->weapon = nullptr;
}

std::vector<Accessory*> CharacterEquipment::getAccessories(AccessoryType accessoryType) const
{
	std::vector<Accessory*> accessories;
	for (const auto &accessory : this->accessories)
	{
		if (accessory->getAccessoryType() == accessoryType)
		{
			accessories.push_back(accessory);
		}
	}

	return accessories;
}

BodyArmor *CharacterEquipment::getBodyArmor(BodyPartName partName) const
{
	return (this->bodyArmors.find(partName) != this->bodyArmors.end()) ?
		this->bodyArmors.at(partName) : nullptr;
}

Shield *CharacterEquipment::getShield() const
{
	return this->shield;
}

std::vector<Trinket*> CharacterEquipment::getTrinkets(TrinketType trinketType) const
{
	std::vector<Trinket*> trinkets;
	for (const auto &trinket : this->trinkets)
	{
		if (trinket->getTrinketType() == trinketType)
		{
			trinkets.push_back(trinket);
		}
	}

	return trinkets;
}

Weapon *CharacterEquipment::getWeapon() const
{
	return this->weapon;
}

int CharacterEquipment::getAccessoryCount(AccessoryType accessoryType) const
{
	int count = 0;
	for (const auto &accessory : this->accessories)
	{
		if (accessory->getAccessoryType() == accessoryType)
		{
			count++;
		}
	}

	return count;
}

int CharacterEquipment::getTrinketCount(TrinketType trinketType) const
{
	int count = 0;
	for (const auto &trinket : this->trinkets)
	{
		if (trinket->getTrinketType() == trinketType)
		{
			count++;
		}
	}

	return count;
}

bool CharacterEquipment::equipAccessory(Accessory *accessory)
{
	DebugAssert(accessory != nullptr);

	// Count how many similar accessories (i.e., rings) are already equipped.
	int similarCount = this->getAccessoryCount(accessory->getAccessoryType());
	bool success = similarCount < accessory->getMaxEquipCount();

	if (success)
	{
		this->accessories.push_back(accessory);
	}

	return success;
}

void CharacterEquipment::equipBodyArmor(BodyArmor *bodyArmor)
{
	DebugAssert(bodyArmor != nullptr);

	if (this->bodyArmors.find(bodyArmor->getPartName()) == this->bodyArmors.end())
	{
		// Make a new BodyPartName -> BodyArmor mapping.
		this->bodyArmors.insert(std::make_pair(bodyArmor->getPartName(), bodyArmor));
	}
	else
	{
		// Replace the old body armor.
		this->bodyArmors.at(bodyArmor->getPartName()) = bodyArmor;
	}
}

void CharacterEquipment::equipShield(Shield *shield)
{
	DebugAssert(shield != nullptr);

	this->shield = shield;
}

bool CharacterEquipment::equipTrinket(Trinket *trinket)
{
	DebugAssert(trinket != nullptr);

	// Count how many similar trinkets (i.e., marks) are already equipped.
	int similarCount = this->getTrinketCount(trinket->getTrinketType());
	bool success = similarCount < trinket->getMaxEquipCount();

	if (success)
	{
		this->trinkets.push_back(trinket);
	}

	return success;
}

void CharacterEquipment::equipWeapon(Weapon *weapon)
{
	DebugAssert(weapon != nullptr);

	this->weapon = weapon;
}

void CharacterEquipment::removeAccessory(int index)
{
	DebugAssert(index >= 0);
	DebugAssert(index < static_cast<int>(this->accessories.size()));

	this->accessories.erase(this->accessories.begin() + index);
}

void CharacterEquipment::removeBodyArmor(BodyPartName partName)
{
	this->bodyArmors.at(partName) = nullptr;
}

void CharacterEquipment::removeShield()
{
	this->shield = nullptr;
}

void CharacterEquipment::removeTrinket(int index)
{
	DebugAssert(index >= 0);
	DebugAssert(index < static_cast<int>(this->trinkets.size()));

	this->trinkets.erase(this->trinkets.begin() + index);
}

void CharacterEquipment::removeWeapon()
{
	this->weapon = nullptr;
}
