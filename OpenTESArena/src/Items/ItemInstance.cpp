#include "ItemInstance.h"

ItemInstance::ItemInstance()
{
	this->defID = -1;
}

void ItemInstance::init(ItemDefinitionID defID)
{
	this->defID = defID;
}
