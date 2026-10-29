#pragma once

#include "ProvinceInstance.h"
#include "ProvinceLibrary.h"

#include "components/utilities/Buffer.h"

struct WorldMapInstance
{
	Buffer<ProvinceInstance> provinces;

	void init(const ProvinceLibrary &provinceLibrary);

	ProvinceInstance &getProvinceFromDefinition(ProvinceDefinitionID defID);
	const ProvinceInstance &getProvinceFromDefinition(ProvinceDefinitionID defID) const;
};
