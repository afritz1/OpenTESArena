#include "WorldMapDefinition.h"

#include "components/debug/Debug.h"

void WorldMapDefinition::init(const BinaryAssetLibrary &binaryAssetLibrary)
{
	this->provinces.clear();
	for (int i = 0; i < CityDataFile::PROVINCE_COUNT; i++)
	{
		ProvinceDefinition provinceDef;
		provinceDef.init(i, binaryAssetLibrary);
		this->provinces.push_back(std::move(provinceDef));
	}
}

int WorldMapDefinition::getProvinceCount() const
{
	return static_cast<int>(this->provinces.size());
}

const ProvinceDefinition &WorldMapDefinition::getProvinceDef(int index) const
{
	DebugAssertIndex(this->provinces, index);
	return this->provinces[index];
}

bool WorldMapDefinition::tryGetProvinceIndex(const ProvinceDefinition &provinceDef,
	int *outProvinceIndex) const
{
	for (int i = 0; i < this->getProvinceCount(); i++)
	{
		const ProvinceDefinition &curProvinceDef = this->getProvinceDef(i);
		if (curProvinceDef.matches(provinceDef))
		{
			*outProvinceIndex = i;
			return true;
		}
	}

	return false;
}
