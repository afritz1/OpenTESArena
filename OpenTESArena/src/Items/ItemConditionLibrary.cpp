#include "ItemConditionLibrary.h"
#include "../Assets/ExeData.h"

#include "components/debug/Debug.h"

void ItemConditionLibrary::init(const ExeData &exeData)
{
	const Span<const std::string> conditionNames = exeData.equipment.itemConditionNames;
	for (int i = 0; i < conditionNames.getCount(); i++)
	{
		ItemConditionDefinition conditionDef;
		conditionDef.init(conditionNames[i].c_str(), -1, -1, -1);
		this->conditionDefs.emplace_back(std::move(conditionDef));
	}
}

int ItemConditionLibrary::getCount() const
{
	return static_cast<int>(this->conditionDefs.size());
}

const ItemConditionDefinition &ItemConditionLibrary::getDefinition(int index) const
{
	DebugAssertIndex(this->conditionDefs, index);
	return this->conditionDefs[index];
}

std::vector<int> ItemConditionLibrary::getDefinitionIndicesIf(const ItemConditionLibraryPredicate &predicate) const
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
