#ifndef LOCATION_H
#define LOCATION_H

#include <string>

// Each instance of a location points to a unique city, town, village, or dungeon.

// There are a couple locations with duplicate names, so to get a unique location, 
// it must be paired with a province name.

// Later this might also have a unique seed ID, so if a location is a city, town, 
// or village, its voxel data can be regenerated.

enum class ClimateName;
enum class LocationType;

class Location
{
private:
	std::string name;
	int provinceID;
	LocationType locationType;
	ClimateName climateName;
public:
	Location(const std::string &name, int provinceID, 
		LocationType locationType, ClimateName climateName);
	~Location();

	int getProvinceID() const;
	LocationType getLocationType() const;
	ClimateName getClimateName() const;
	const std::string &getName() const;
	std::string typeToString() const;
};

#endif
