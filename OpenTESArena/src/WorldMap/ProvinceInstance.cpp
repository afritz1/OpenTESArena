#include "ProvinceDefinition.h"
#include "ProvinceInstance.h"

#include "components/debug/Debug.h"

void ProvinceInstance::init(ProvinceDefinitionID provinceDefID)
{
	this->provinceDefID = provinceDefID;

	const ProvinceDefinition &provinceDef = ProvinceLibrary::getInstance().getDefinition(provinceDefID);
	const int locationCount = static_cast<int>(provinceDef.locations.size());
	this->locations.init(locationCount);

	for (int i = 0; i < locationCount; i++)
	{
		const LocationDefinition &locationDef = provinceDef.locations[i];

		LocationInstance locationInst;
		locationInst.init(i, locationDef);
		this->locations.set(i, std::move(locationInst));
	}
}
