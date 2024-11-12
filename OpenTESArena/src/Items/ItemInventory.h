#ifndef ITEM_INVENTORY_H
#define ITEM_INVENTORY_H

#include <vector>

#include "ItemDefinition.h"
#include "ItemInstance.h"

class ItemInventory
{
private:
	std::vector<ItemInstance> items;
public:
	int getTotalSlotCount() const;
	int getEmptySlotCount() const;
	int getOccupiedSlotCount() const;
	ItemInstance &getSlot(int index);
	const ItemInstance &getSlot(int index) const;

	double getWeight() const;

	bool findFirstEmptySlot(int *outIndex) const;
	bool findFirstSlot(ItemDefinitionID defID, int *outIndex) const;
	bool findLastSlot(ItemDefinitionID defID, int *outIndex) const;

	bool insert(ItemDefinitionID defID); // @todo: attempt stacking if possible.
	// @todo: if inserting multiple, want some kind of "ItemInventoryTransferResult" struct that holds what didn't go

	// @todo: sort() or shrink()
	void clear();
};

#endif
