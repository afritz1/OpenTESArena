#ifndef ITEM_MATERIAL_LIBRARY_H
#define ITEM_MATERIAL_LIBRARY_H

#include <functional>
#include <vector>

#include "ItemDefinition.h"

#include "components/utilities/Singleton.h"

struct ExeData;

using ItemMaterialLibraryPredicate = std::function<bool(const ItemMaterialDefinition &materialDef)>;

// Stores all item materials for the game. Intended for weapons and armor.
class ItemMaterialLibrary : public Singleton<ItemMaterialLibrary>
{
private:
	std::vector<ItemMaterialDefinition> materialDefs;
public:
	void init(const ExeData &exeData);

	int getCount() const;
	const ItemMaterialDefinition &getDefinition(int index) const;
	std::vector<int> getDefinitionIndicesIf(const ItemMaterialLibraryPredicate &predicate) const;
};

#endif
