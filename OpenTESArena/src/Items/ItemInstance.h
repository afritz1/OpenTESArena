#ifndef ITEM_INSTANCE_H
#define ITEM_INSTANCE_H

#include "ItemDefinition.h"

// Represents a slot in an inventory, supports stacking.
struct ItemInstance
{
	ItemDefinitionID defID;

	ItemInstance();

	void init(ItemDefinitionID defID);

	bool isValid() const;
};

#endif
