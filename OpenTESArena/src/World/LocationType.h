#ifndef LOCATION_TYPE_H
#define LOCATION_TYPE_H

// Each location on a province map has a type.

enum class LocationType
{
	CityState,
	Town,
	Village,
	StaffDungeon,
	StaffMapDungeon,
	NamedDungeon
};

#endif
