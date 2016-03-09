#ifndef LOCATION_TYPE_H
#define LOCATION_TYPE_H

// A location type is for classifying the size or category of a place on the map.
// It should be used to determine the size of a randomly generated location with
// respect to city-like places. 

// The player's map should use these elements to determine the icon used.

enum class LocationType
{
	// City-like places.
	CityState,
	Town,
	Village,

	// Dungeon-like places. Each main quest has a pair of dungeons. The first in 
	// each pair is here, like "Stonekeep", which has the generic dungeon icon.
	Dungeon,

	// "Unique" main quest places. These ones all have distinct icons. When 
	// checking what icon to get and the type is Unique, then look at the
	// LocationName instead.
	Unique
};

#endif