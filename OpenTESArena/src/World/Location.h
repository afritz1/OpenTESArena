#ifndef LOCATION_H
#define LOCATION_H

#include <string>
#include <vector>

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

	const std::string &getDisplayName() const;
	const ProvinceName &getProvinceName() const;
	const LocationType &getLocationType() const;
	const ClimateName &getClimateName() const;
	std::string typeToString() const;
};

#endif
