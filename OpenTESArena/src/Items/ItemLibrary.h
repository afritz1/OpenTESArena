#ifndef ITEM_LIBRARY_H
#define ITEM_LIBRARY_H

#include <functional>
#include <vector>

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

	int getCount() const;
	const ItemDefinition &getDefinition(int index) const;
	std::vector<int> getDefinitionIndicesIf(const ItemLibraryPredicate &predicate) const;
};

#endif
