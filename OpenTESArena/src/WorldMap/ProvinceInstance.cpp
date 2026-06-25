#include "ProvinceInstance.h"

#include "components/debug/Debug.h"

void ProvinceInstance::init(int provinceIndex, const ProvinceDefinition &provinceDef)
{
	this->provinceIndex = provinceIndex;

	this->locations.clear();
	for (int i = 0; i < provinceDef.getLocationCount(); i++)
	{
		const LocationDefinition &locationDef = provinceDef.getLocationDef(i);

		LocationInstance locationInst;
		locationInst.init(i, locationDef);
		this->locations.emplace_back(std::move(locationInst));
	}
}

int ProvinceInstance::getProvinceIndex() const
{
	return this->provinceIndex;
}

int ProvinceInstance::getLocationCount() const
{
	return static_cast<int>(this->locations.size());
}

LocationInstance &ProvinceInstance::getLocationInstance(int index)
{
	DebugAssertIndex(this->locations, index);
	return this->locations[index];
}

const LocationInstance &ProvinceInstance::getLocationInstance(int index) const
{
	DebugAssertIndex(this->locations, index);
	return this->locations[index];
}
