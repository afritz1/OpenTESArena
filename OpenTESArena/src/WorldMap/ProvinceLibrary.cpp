#include "ProvinceDefinition.h"
#include "ProvinceLibrary.h"
#include "../Assets/BinaryAssetLibrary.h"

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

const ProvinceDefinition &ProvinceLibrary::getProvinceDef(int index) const
{
	DebugAssertIndex(this->provinces, index);
	return this->provinces[index];
}
