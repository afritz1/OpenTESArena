#include "ItemInventory.h"
#include "ItemLibrary.h"

#include "components/debug/Debug.h"

void ItemInventory::clear()
{
	this->items.clear();
}

int ItemInventory::getTotalSlotCount() const
{
	return static_cast<int>(this->items.size());
}

int ItemInventory::getEmptySlotCount() const
{
	int count = 0;
	for (int i = 0; i < this->getTotalSlotCount(); i++)
	{
		const ItemInstance &itemInst = this->getSlot(i);
		if (itemInst.defID == -1)
		{
			count++;
		}
	}

	return count;
}

int ItemInventory::getOccupiedSlotCount() const
{
	return this->getTotalSlotCount() - this->getEmptySlotCount();
}

ItemInstance &ItemInventory::getSlot(int index)
{
	DebugAssertIndex(this->items, index);
	return this->items[index];
}

const ItemInstance &ItemInventory::getSlot(int index) const
{
	DebugAssertIndex(this->items, index);
	return this->items[index];
}

double ItemInventory::getWeight() const
{
	const ItemLibrary &itemLibrary = ItemLibrary::getInstance();

	double totalWeight = 0.0;
	for (int i = 0; i < this->getTotalSlotCount(); i++)
	{
		const ItemInstance &itemInst = this->getSlot(i);
		const ItemDefinitionID itemDefID = itemInst.defID;
		if (itemDefID == -1)
		{
			continue;
		}

		const ItemDefinition &itemDef = itemLibrary.getDefinition(itemDefID);
		totalWeight += itemDef.getWeight();
	}

	return totalWeight;
}

bool ItemInventory::findFirstEmptySlot(int *outIndex) const
{
	for (int i = 0; i < this->getTotalSlotCount(); i++)
	{
		const ItemInstance &itemInst = this->getSlot(i);
		if (itemInst.defID == -1)
		{
			*outIndex = i;
			return true;
		}
	}

	return false;
}

bool ItemInventory::findFirstSlot(ItemDefinitionID defID, int *outIndex) const
{
	for (int i = 0; i < this->getTotalSlotCount(); i++)
	{
		const ItemInstance &itemInst = this->getSlot(i);
		if (itemInst.defID == defID)
		{
			*outIndex = i;
			return true;
		}
	}

	return false;
}

bool ItemInventory::findLastSlot(ItemDefinitionID defID, int *outIndex) const
{
	for (int i = this->getTotalSlotCount() - 1; i >= 0; i--)
	{
		const ItemInstance &itemInst = this->getSlot(i);
		if (itemInst.defID == defID)
		{
			*outIndex = i;
			return true;
		}
	}

	return false;
}

bool ItemInventory::insert(ItemDefinitionID defID)
{
	int insertIndex;
	if (!this->findFirstEmptySlot(&insertIndex))
	{
		return false;
	}

	if (insertIndex < this->getTotalSlotCount())
	{
		ItemInstance &existingItemInst = this->getSlot(insertIndex);
		existingItemInst.init(defID);
	}
	else
	{
		ItemInstance &newItemInst = this->items.emplace_back(ItemInstance());
		newItemInst.init(defID);
	}

	return true;
}
