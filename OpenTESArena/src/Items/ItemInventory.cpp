#include "ItemInventory.h"
#include "ItemLibrary.h"

#include "components/debug/Debug.h"

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
		if (!itemInst.isValid())
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
		if (!itemInst.isValid())
		{
			continue;
		}

		const ItemDefinition &itemDef = itemLibrary.getDefinition(itemInst.defID);
		totalWeight += itemDef.getWeight();
	}

	return totalWeight;
}

int ItemInventory::getCountOf(ItemDefinitionID defID) const
{
	int count = 0;
	for (int i = 0; i < this->getTotalSlotCount(); i++)
	{
		const ItemInstance &itemInst = this->getSlot(i);
		if (itemInst.isValid() && (itemInst.defID == defID))
		{
			count += itemInst.stackAmount;
		}
	}

	return count;
}

bool ItemInventory::findFirstEmptySlot(int *outIndex) const
{
	for (int i = 0; i < this->getTotalSlotCount(); i++)
	{
		const ItemInstance &itemInst = this->getSlot(i);
		if (!itemInst.isValid())
		{
			*outIndex = i;
			return true;
		}
	}

	*outIndex = -1;
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

	*outIndex = -1;
	return false;
}

bool ItemInventory::findFirstSlotIf(const ItemInventoryPredicate &predicate, int *outIndex) const
{
	for (int i = 0; i < this->getTotalSlotCount(); i++)
	{
		const ItemInstance &itemInst = this->getSlot(i);
		if (!itemInst.isValid())
		{
			continue;
		}

		const ItemDefinition &itemDef = ItemLibrary::getInstance().getDefinition(itemInst.defID);
		if (predicate(itemDef))
		{
			*outIndex = i;
			return true;
		}
	}

	*outIndex = -1;
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

	*outIndex = -1;
	return false;
}

void ItemInventory::insert(ItemDefinitionID defID, int stackAmount)
{
	DebugAssert(defID >= 0);
	DebugAssert(stackAmount >= 1);

	const ItemDefinition &itemDef = ItemLibrary::getInstance().getDefinition(defID);

	int insertIndex = -1;
	int totalStackAmount = stackAmount;
	if (itemDef.isStackable)
	{
		if (this->findFirstSlot(defID, &insertIndex))
		{
			ItemInstance &existingItemInst = this->getSlot(insertIndex);
			totalStackAmount += existingItemInst.stackAmount;
		}
	}

	if (insertIndex < 0)
	{
		if (!this->findFirstEmptySlot(&insertIndex))
		{
			insertIndex = static_cast<int>(this->items.size());
			this->items.emplace_back(ItemInstance());
		}
	}

	ItemInstance &itemInst = this->getSlot(insertIndex);
	itemInst.init(defID);
	itemInst.stackAmount = totalStackAmount;
}

void ItemInventory::compact()
{
	for (int i = 0; i < this->getTotalSlotCount(); i++)
	{
		ItemInstance &itemInst = this->getSlot(i);
		if (itemInst.isValid())
		{
			int emptyIndex;
			if (this->findFirstEmptySlot(&emptyIndex) && (emptyIndex < i))
			{
				ItemInstance &destinationItemInst = this->getSlot(emptyIndex);
				destinationItemInst.defID = itemInst.defID;
				destinationItemInst.isEquipped = itemInst.isEquipped;
				itemInst.clear();
			}
		}
	}
}

void ItemInventory::clear()
{
	this->items.clear();
}
