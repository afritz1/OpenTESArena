#include <cassert>

#include "Consumable.h"

#include "ItemType.h"

Consumable::Consumable()
	: Item(nullptr)
{
	
}

Consumable::~Consumable()
{

}

ItemType Consumable::getItemType() const
{
	return ItemType::Consumable;
}
