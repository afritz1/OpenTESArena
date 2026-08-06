#pragma once

#include <functional>
#include <vector>

#include "ItemDefinition.h"
#include "ItemInstance.h"

using ItemDefinitionPredicate = std::function<bool(const ItemDefinition&)>;
using ItemInstancePredicate = std::function<bool(const ItemInstance&)>;

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
	bool findFirstValidSlotIf(const ItemInstancePredicate &predicate, int *outIndex) const;
	bool findFirstValidSlotDefIf(const ItemDefinitionPredicate &predicate, int *outIndex) const;
	bool findLastSlot(ItemDefinitionID defID, int *outIndex) const;

	void insert(ItemDefinitionID defID, int stackAmount = 1);
	void compact();
	void clear();
};
