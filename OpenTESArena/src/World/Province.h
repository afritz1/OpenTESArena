#ifndef PROVINCE_H
#define PROVINCE_H

#include <string>
#include <vector>

#include "Location.h"
#include "ProvinceName.h"

// After checking through some climates, it feels like it would make sense to
// give each province a "typical weather", like rain and fog for Black Marsh,
// even though several of Black Marsh's locations are deserts. Maybe the
// developers intended it to be like wastelands, or barrens?

enum class CharacterRaceName;

class Province
{
private:
	ProvinceName provinceName;
	std::vector<Location> locations;
public:
	Province(ProvinceName provinceName);
	~Province();

	const ProvinceName &getProvinceName() const;
	const std::vector<Location> &getLocations() const;
	std::string toString() const;
	std::string getRaceDisplayName(bool plural) const;
	CharacterRaceName getRaceName() const;

	void addLocation(const Location &location);
};

#endif
