#ifndef PROVINCE_DEFINITION_H
#define PROVINCE_DEFINITION_H

#include <vector>

#include "LocationDefinition.h"
#include "../Assets/CityDataFile.h"
#include "../Assets/ExeData.h"
#include "../Math/Rect.h"

class ProvinceDefinition
{
private:
	std::vector<LocationDefinition> locations;
	std::string name;
	int globalX, globalY, globalW, globalH; // Province-to-world-map projection.
public:
	// Initialize from original game data.
	void init(int provinceID, const CityDataFile::ProvinceData &provinceData,
		const ExeData::CityGeneration &cityGen);
	// @todo: eventually have init(const char *filename) for custom provinces.

	// Gets the number of locations in the province.
	int getLocationCount() const;

	// Gets the location definition at the given index.
	const LocationDefinition &getLocationDef(int index) const;

	// Gets the display name of the province.
	const std::string &getName() const;
	
	// Creates a rectangle from the province's global X, Y, W, and H values.
	Rect getGlobalRect() const;
};

#endif
