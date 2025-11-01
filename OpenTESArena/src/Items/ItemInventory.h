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

	int getCountOf(ItemDefinitionID defID) const;

	bool findFirstEmptySlot(int *outIndex) const;
	bool findFirstSlot(ItemDefinitionID defID, int *outIndex) const;
	bool findLastSlot(ItemDefinitionID defID, int *outIndex) const;

	void insert(ItemDefinitionID defID, int stackAmount = 1);
	void compact();
	void clear();
};

#endif
