#pragma once

#include "ProvinceInstance.h"
#include "ProvinceLibrary.h"

#include "components/utilities/Buffer.h"

class WorldMapInstance
{
private:
	Buffer<ProvinceInstance> provinces;
public:
	void init(const ProvinceLibrary &provinceLibrary);

	// Gets the number of provinces in the world map.
	int getProvinceCount() const;

	// Gets the province instance at the given index.
	ProvinceInstance &getProvinceInstance(int index);
	const ProvinceInstance &getProvinceInstance(int index) const;
};
