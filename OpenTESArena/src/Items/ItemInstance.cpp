#include "ItemInstance.h"

ItemInstance::ItemInstance()
{
	this->defID = -1;
	this->isEquipped = false;
}

void ItemInstance::init(ItemDefinitionID defID)
{
	this->defID = defID;
	this->isEquipped = false;
}

bool ItemInstance::isValid() const
{
	return this->defID >= 0;
}

void ItemInstance::clear()
{
	this->defID = -1;
	this->isEquipped = false;
}
