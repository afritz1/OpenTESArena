#ifndef ITEM_TYPE_H
#define ITEM_TYPE_H

// Items are anything that would belong in the player's inventory, other character's 
// inventories, containers (i.e., piles), or merchants' lists. The "Item" class 
// should have a virtual method for "getItemType()" that is implemented by all the 
// derived classes as a simple return.

// There shouldn't be a need at any point to do something like:
// if (type == EntityType::something) { dynamic_cast<DerivedItem>(item); ...; }
// I think I just need to find a common "interaction" interface, instead of common
// behavior, because they don't all have that much common behavior. Just mess around
// with some adjective interfaces and see what allows them all to work together.

// Hmm... I'm not sure how to design it in a way that wouldn't have, for example, a 
// list of Items that are checked for their type and then used with something specific. 
// An Item shouldn't have virtual methods present in all the derived types, but in that 
// case, a dynamic_cast wouldn't be necessary in order to get the desired methods. 
// However, it doesn't make a lot of sense to give the ConsumableItem class a "repair()" 
// or "swing()" method.

// It costs 1/10 of the total worth of an item to identify it. Hmm... this would imply 
// that the wizard knows what it is, and chooses their own price for what they think 
// it's worth, in a way.

// All items should have the capabilty of being a quest item.

// Accessories are: amulets, bracers, bracelets, rings, etc. (excluding crystals and marks).
// Armor are: helmet, cuirass, gauntlets, shield, etc..
// Consumables are: potions, food, etc..
// Miscellaneous are: useless items for selling (Crafting? Probably not).
// Trinkets are: crystals and marks (non-metal accessories).
// Weapons are: long swords, short shorts, bows, maces, etc..

enum class ItemType
{
	Accessory,
	Armor,
	Consumable,
	Miscellaneous,
	Trinket,
	Weapon
};

#endif