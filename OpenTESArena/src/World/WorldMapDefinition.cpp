#include "WorldMapDefinition.h"

#include "components/debug/Debug.h"

void WorldMapDefinition::init(const CityDataFile &cityData)
{
	for (int i = 0; i < CityDataFile::PROVINCE_COUNT; i++)
	{
		const CityDataFile::ProvinceData &provinceData = cityData.getProvinceData(i);

		ProvinceDefinition provinceDef;
		provinceDef.init(provinceData);
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

void WorldMapDefinition::clear()
{
	this->provinces.clear();
}
