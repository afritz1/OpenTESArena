#ifndef ITEM_INSTANCE_H
#define ITEM_INSTANCE_H

#include "ItemDefinition.h"

struct ItemInstance
{
	ItemDefinitionID defID;

	ItemInstance();

	void init(ItemDefinitionID defID);
};

#endif
