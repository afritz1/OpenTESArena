#include <cassert>

#include "ItemParser.h"

#include "Accessory.h"
#include "AccessoryType.h"
#include "ArmorMaterialType.h"
#include "ArmorType.h"
#include "BodyArmor.h"
#include "Food.h"
#include "HeavyArmorMaterial.h"
#include "Item.h"
#include "ItemType.h"
#include "LightArmorMaterial.h"
#include "MediumArmorMaterial.h"
#include "MetalType.h"
#include "MiscellaneousItem.h"
#include "MiscellaneousItemType.h"
#include "Potion.h"
#include "Shield.h"
#include "ShieldType.h"
#include "Trinket.h"
#include "TrinketType.h"
#include "Weapon.h"
#include "WeaponType.h"
#include "../Utilities/File.h"

// These paths might be obsolete soon.
const std::string ItemParser::PATH = "data/text/";
const std::string ItemParser::FILENAME = "items.txt";

std::vector<std::unique_ptr<Item>> ItemParser::parse()
{
	std::string fullPath(ItemParser::PATH + ItemParser::FILENAME);

	// Read the artifacts file into a string.
	std::string text = File::toString(fullPath);

	// Parsing code here... pushing items into the vector.
	// ...
	// ...

	return std::vector<std::unique_ptr<Item>>();
}
