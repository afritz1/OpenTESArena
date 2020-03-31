#include "WorldMapInstance.h"

#include "components/debug/Debug.h"

void WorldMapInstance::init(const WorldMapDefinition &worldMapDef)
{
	this->provinces.clear();
	for (int i = 0; i < worldMapDef.getProvinceCount(); i++)
	{
		const ProvinceDefinition &provinceDef = worldMapDef.getProvinceDef(i);

		ProvinceInstance provinceInst;
		provinceInst.init(i, provinceDef);
		this->provinces.push_back(std::move(provinceInst));
	}
}

int WorldMapInstance::getProvinceCount() const
{
	return static_cast<int>(this->provinces.size());
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
