#ifndef ITEM_CONDITION_LIBRARY_H
#define ITEM_CONDITION_LIBRARY_H

#include <functional>
#include <vector>

#include "components/utilities/Singleton.h"

#include "ItemDefinition.h"

class ExeData;

using ItemConditionLibraryPredicate = std::function<bool(const ItemConditionDefinition &conditionDef)>;

// Stores all item conditions for the game. Intended for items that degrade with use.
class ItemConditionLibrary : public Singleton<ItemConditionLibrary>
{
private:
	std::vector<ItemConditionDefinition> conditionDefs;
public:
	void init(const ExeData &exeData);

	int getCount() const;
	const ItemConditionDefinition &getDefinition(int index) const;
	std::vector<int> getDefinitionIndicesIf(const ItemConditionLibraryPredicate &predicate) const;
};

#endif
