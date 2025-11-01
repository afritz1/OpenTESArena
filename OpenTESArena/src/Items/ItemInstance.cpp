#include "ItemInstance.h"

#include "components/debug/Debug.h"

ItemInstance::ItemInstance()
{
	this->defID = -1;
	this->stackAmount = -1;
	this->isEquipped = false;
}

void ItemInstance::init(ItemDefinitionID defID, int stackAmount)
{
	DebugAssert(stackAmount >= 1);
	this->defID = defID;
	this->stackAmount = stackAmount;
	this->isEquipped = false;
}

void ItemInstance::init(ItemDefinitionID defID)
{
	this->init(defID, 1);
}

bool ItemInstance::isValid() const
{
	return (this->defID >= 0) && (this->stackAmount >= 1);
}

void ItemInstance::clear()
{
	this->defID = -1;
	this->stackAmount = -1;
	this->isEquipped = false;
}
