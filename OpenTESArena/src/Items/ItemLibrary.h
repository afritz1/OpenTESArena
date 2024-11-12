#ifndef ITEM_LIBRARY_H
#define ITEM_LIBRARY_H

#include <functional>

#include "components/utilities/Singleton.h"

#include "ItemDefinition.h"

class ExeData;

using ItemLibraryPredicate = std::function<bool(const ItemDefinition &itemDef)>;

// Stores all item definitions for the game.
class ItemLibrary : public Singleton<ItemLibrary>
{
private:
	std::vector<ItemDefinition> itemDefs;
public:
	void init(const ExeData &exeData);

	int getItemDefCount() const;
	const ItemDefinition &getItemDef(int index) const;
	std::vector<int> getItemDefIndicesIf(const ItemLibraryPredicate &predicate) const;
};

#endif
