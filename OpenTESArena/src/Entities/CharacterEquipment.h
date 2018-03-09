#ifndef CHARACTER_EQUIPMENT_H
#define CHARACTER_EQUIPMENT_H

#include <unordered_map>
#include <vector>

// A character equipment object is designed for humanoids with a head, chest, two
// shoulders, two arms, two legs, and two feet.

// All of the pointer members refer to inventory items. I didn't use "weak_ptr"
// or something similar because raw pointers are behaving as intended here, and 
// there is no memory management to worry about.

// Null is valid to return when nothing is equipped in a slot. Returning a vector
// with a size of zero is also valid.

// I'm not sure how indexing is going to work, because Arena's equipped items don't 
// have a distinct spot in the interface like Daggerfall does (i.e., ring #1 and #2), 
// but they do have an order that they are equipped (first in, first out). However, 
// unequipping a particular item by clicking on it may not be straightforward to 
// code. Display name matching in the "getAccessories()" or "getTrinkets()" vectors 
// should be sufficient for solving that problem, and duplicates will simply get 
// one of them unequipped.

class Accessory;
class BodyArmor;
class Shield;
class Trinket;
class Weapon;

enum class AccessoryType;
enum class ArmorType;
enum class BodyPartName;
enum class TrinketType;

namespace std
{
	// Hash specialization, required until GCC 6.1.
	template <>
	struct hash<BodyPartName>
	{
		size_t operator()(const BodyPartName &x) const
		{
			return static_cast<size_t>(x);
		}
	};
}

class CharacterEquipment
{
private:
	std::vector<Accessory*> accessories;
	std::unordered_map<BodyPartName, BodyArmor*> bodyArmors;
	Shield *shield;
	std::vector<Trinket*> trinkets;
	Weapon *weapon;
public:
	// Initial "paper doll" with nothing.
	CharacterEquipment();
	~CharacterEquipment();

	std::vector<Accessory*> getAccessories(AccessoryType accessoryType) const;
	BodyArmor *getBodyArmor(BodyPartName partName) const;
	Shield *getShield() const;
	std::vector<Trinket*> getTrinkets(TrinketType trinketType) const;
	Weapon *getWeapon() const;

	// Count methods, for determining how many of a given type are already equipped.
	// These are only required for accessories and trinkets, because they might be
	// plural.
	int getAccessoryCount(AccessoryType accessoryType) const;
	int getTrinketCount(TrinketType trinketType) const;

	// Equip methods. All of these assume that the character's class is allowed to 
	// equip the item. Each method that returns a boolean is returning whether there
	// was room to equip the item (i.e., multiple rings). The void methods simply 
	// replace the old item.
	bool equipAccessory(Accessory *accessory);
	void equipBodyArmor(BodyArmor *bodyArmor);
	void equipShield(Shield *shield);
	bool equipTrinket(Trinket *trinket);
	void equipWeapon(Weapon *weapon);

	// Remove item. For methods using an index, this assumes that the caller knows 
	// how to find the index of the item they wish to remove.
	void removeAccessory(int index);
	void removeBodyArmor(BodyPartName partName);
	void removeShield();
	void removeTrinket(int index);
	void removeWeapon();
};

#endif
