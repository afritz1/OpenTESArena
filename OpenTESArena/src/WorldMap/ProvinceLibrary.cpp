#include "ProvinceDefinition.h"
#include "ProvinceLibrary.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Math/Random.h"

#include "components/debug/Debug.h"

void ProvinceLibrary::init(const BinaryAssetLibrary &binaryAssetLibrary)
{
	constexpr int provinceCount = CityDataFile::PROVINCE_COUNT;
	this->provinces.init(provinceCount);

	for (int i = 0; i < provinceCount; i++)
	{
		ProvinceDefinition provinceDef;
		provinceDef.init(i, binaryAssetLibrary);
		this->provinces.set(i, std::move(provinceDef));
	}
}

int ProvinceLibrary::getProvinceCount() const
{
	return this->provinces.getCount();
}

const ProvinceDefinition &ProvinceLibrary::getDefinition(ProvinceDefinitionID id) const
{
	DebugAssertIndex(this->provinces, id);
	return this->provinces[id];
}

ProvinceDefinitionID ProvinceLibrary::getRandomDefinitionIdIf(Random &random, const ProvinceDefinitionPredicate &predicate) const
{
	std::vector<int> possibleIndices;
	for (int i = 0; i < this->provinces.getCount(); i++)
	{
		if (predicate(this->provinces[i]))
		{
			possibleIndices.emplace_back(i);
		}
	}

	if (possibleIndices.empty())
	{
		return -1;
	}

	const int randomIndex = possibleIndices[random.next(static_cast<int>(possibleIndices.size()))];
	return static_cast<ProvinceDefinitionID>(randomIndex);
}

ProvinceDefinitionID ProvinceLibrary::findDefinitionIdIf(const ProvinceDefinitionPredicate &predicate) const
{
	for (int i = 0; i < this->provinces.getCount(); i++)
	{
		if (predicate(this->provinces[i]))
		{
			return i;
		}
	}

	return -1;
}
