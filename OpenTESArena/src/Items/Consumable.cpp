#include <cassert>

#include "Consumable.h"
#include "ItemType.h"

Consumable::Consumable()
	: Item(nullptr)
{
	
}

ItemType Consumable::getItemType() const
{
	return ItemType::Consumable;
}
