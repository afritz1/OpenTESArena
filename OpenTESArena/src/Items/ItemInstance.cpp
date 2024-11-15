#include "ItemInstance.h"

ItemInstance::ItemInstance()
{
	this->defID = -1;
}

void ItemInstance::init(ItemDefinitionID defID)
{
	this->defID = defID;
}

bool ItemInstance::isValid() const
{
	return this->defID >= 0;
}
