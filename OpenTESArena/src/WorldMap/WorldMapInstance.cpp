#include "WorldMapInstance.h"

#include "components/debug/Debug.h"

void WorldMapInstance::init(const ProvinceLibrary &provinceLibrary)
{
	const int provinceCount = provinceLibrary.getProvinceCount();
	this->provinces.init(provinceCount);

	for (int i = 0; i < provinceCount; i++)
	{
		const ProvinceDefinitionID provinceDefID = provinceLibrary.findDefinitionIdIf(
			[i](const ProvinceDefinition &provinceDef)
		{
			return provinceDef.originalProvinceIndex == i;
		});

		ProvinceInstance provinceInst;
		provinceInst.init(provinceDefID);
		this->provinces.set(i, std::move(provinceInst));
	}
}

ProvinceInstance &WorldMapInstance::getProvinceFromDefinition(ProvinceDefinitionID defID)
{
	for (ProvinceInstance &inst : this->provinces)
	{
		if (inst.provinceDefID == defID)
		{
			return inst;
		}
	}

	DebugLogErrorFormat("Couldn't find province instance with definition ID %d.", defID);
	return this->provinces[0];
}

const ProvinceInstance &WorldMapInstance::getProvinceFromDefinition(ProvinceDefinitionID defID) const
{
	for (const ProvinceInstance &inst : this->provinces)
	{
		if (inst.provinceDefID == defID)
		{
			return inst;
		}
	}

	DebugLogErrorFormat("Couldn't find province instance with definition ID %d.", defID);
	return this->provinces[0];
}
