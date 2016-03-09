#include <cassert>

#include "Armor.h"
#include "ArmorMaterial.h"
#include "ItemType.h"

Armor::Armor()
{
	
}

Armor::~Armor()
{

}

ItemType Armor::getItemType() const
{
	return ItemType::Armor;
}
