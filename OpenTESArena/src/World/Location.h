#ifndef LOCATION_H
#define LOCATION_H

#include <string>

enum class ClimateName;
enum class LocationType;
enum class ProvinceName;

class Location
{
private:
	std::string name;
	ProvinceName provinceName;
	LocationType locationType;
	ClimateName climateName;
public:
	Location(const std::string &name, ProvinceName provinceName, 
		LocationType locationType, ClimateName climateName);
	~Location();

	ProvinceName getProvinceName() const;
	LocationType getLocationType() const;
	ClimateName getClimateName() const;
	const std::string &getDisplayName() const;
	std::string typeToString() const;
};

#endif
