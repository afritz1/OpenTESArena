#include "WorldMapInstance.h"

#include "components/debug/Debug.h"

void WorldMapInstance::init(const ProvinceLibrary &provinceLibrary)
{
	const int provinceCount = provinceLibrary.getProvinceCount();
	this->provinces.init(provinceCount);

	for (int i = 0; i < provinceCount; i++)
	{
		const ProvinceDefinition &provinceDef = provinceLibrary.getProvinceDef(i);

		ProvinceInstance provinceInst;
		provinceInst.init(i, provinceDef);
		this->provinces.set(i, std::move(provinceInst));
	}
}

int WorldMapInstance::getProvinceCount() const
{
	return this->provinces.getCount();
}

ProvinceInstance &WorldMapInstance::getProvinceInstance(int index)
{
	DebugAssertIndex(this->provinces, index);
	return this->provinces[index];
}

const ProvinceInstance &WorldMapInstance::getProvinceInstance(int index) const
{
	DebugAssertIndex(this->provinces, index);
	return this->provinces[index];
}
