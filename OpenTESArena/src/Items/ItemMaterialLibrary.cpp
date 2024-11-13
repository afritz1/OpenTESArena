#include "ItemMaterialLibrary.h"
#include "../Assets/ExeData.h"

#include "components/debug/Debug.h"

void ItemMaterialLibrary::init(const ExeData &exeData)
{
	const BufferView<const std::string> materialNames = exeData.equipment.materialNames;
	for (int i = 0; i < materialNames.getCount(); i++)
	{
		ItemMaterialDefinition materialDef;
		materialDef.init(materialNames[i].c_str(), -1, -1, -1);
		this->materialDefs.emplace_back(std::move(materialDef));
	}
}

int ItemMaterialLibrary::getCount() const
{
	return static_cast<int>(this->materialDefs.size());
}

const ItemMaterialDefinition &ItemMaterialLibrary::getDefinition(int index) const
{
	DebugAssertIndex(this->materialDefs, index);
	return this->materialDefs[index];
}

std::vector<int> ItemMaterialLibrary::getDefinitionIndicesIf(const ItemMaterialLibraryPredicate &predicate) const
{
	std::vector<int> indices;
	for (int i = 0; i < this->getCount(); i++)
	{
		if (predicate(this->getDefinition(i)))
		{
			indices.emplace_back(i);
		}
	}

	return indices;
}
