#ifndef CHARACTER_EQUIPMENT_H
#define CHARACTER_EQUIPMENT_H

#include <map>
#include <memory>
#include <vector>

// A character equipment object is purely for humanoids with a head, chest, two
// shoulders, two arms, two legs, and two feet.

// None of the members are unique_ptrs because they are references to items in the
// inventory object. None of the members are const either, because equipped items 
// can change based on usage.

// Container classes like an inventory should have shared_ptrs of everything, and 
// users of those items, like the equipment class, get weak_ptrs to the items. Those 
// weak_ptrs might also be returned for get methods.

class Accessory;
class BodyArmor;
class Shield;
class Trinket;
class Weapon;

enum class AccessoryType;
enum class ArmorType;
enum class BodyPartName;
enum class TrinketName;

class CharacterEquipment
{
private:
	// I don't think these should be shared_ptrs. The design of the inventory
	// should guarantee that all items will outlive any weak references to them.
	std::vector<std::weak_ptr<Accessory>> accessories;
	std::map<BodyPartName, std::weak_ptr<BodyArmor>> bodyArmors;
	std::weak_ptr<Shield> shield;
	std::vector<std::weak_ptr<Trinket>> trinkets;
	std::weak_ptr<Weapon> weapon;
public:
	// Initial "paper doll" with nothing.
	CharacterEquipment();
	~CharacterEquipment();

	// Using pointers allows for returning null, since that is a valid value for
	// when nothing is equipped in the slot. Also, returning vectors allows for
	// checking if the size is zero, which is equivalent to "null". Vectors are
	// used because there might be multiple items in the same type of slot, like
	// with bracelets.
	std::vector<std::weak_ptr<Accessory>> getAccessories(AccessoryType accessoryType) const;
	std::weak_ptr<BodyArmor> getBodyArmor(BodyPartName partName) const;
	std::weak_ptr<Shield> getShield() const;
	std::vector<std::weak_ptr<Trinket>> getTrinkets(TrinketName trinketName) const;
	std::weak_ptr<Weapon> getWeapon() const;

	// Add item... set this weak_ptr to the given shared_ptr (or weak_ptr?) only if
	// there is room to equip the item.

	// Remove item (with index for items in a vector)... it should set this weak_ptr to null.
};

#endif
