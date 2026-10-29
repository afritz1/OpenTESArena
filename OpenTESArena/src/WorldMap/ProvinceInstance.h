#pragma once

#include <vector>

#include "LocationInstance.h"
#include "ProvinceLibrary.h"

#include "components/utilities/Buffer.h"

struct ProvinceInstance
{
	Buffer<LocationInstance> locations;
	ProvinceDefinitionID provinceDefID;

	void init(ProvinceDefinitionID provinceDefID);
};
