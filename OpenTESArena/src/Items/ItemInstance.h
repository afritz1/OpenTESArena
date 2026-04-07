#pragma once

#include "ItemDefinition.h"

// Represents a slot in an inventory, supports stacking.
struct ItemInstance
{
	ItemDefinitionID defID;
	int stackAmount;
	bool isEquipped;

	ItemInstance();

	void init(ItemDefinitionID defID, int stackAmount);
	void init(ItemDefinitionID defID);

	bool isValid() const;

	void clear();
};
