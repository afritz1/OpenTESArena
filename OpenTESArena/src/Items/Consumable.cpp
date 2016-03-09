#include "Consumable.h"
#include "ItemType.h"

Consumable::Consumable()
{
	
}

Consumable::~Consumable()
{

}

ItemType Consumable::getItemType() const
{
	return ItemType::Consumable;
}
