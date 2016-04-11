#ifndef CLIMATE_NAME_H
#define CLIMATE_NAME_H

// A climate is set for each individual location. It is not dependent on the 
// province, and may or may not have a dependence on geographical location, since 
// deserts seem to be scattered around; not just in Hammerfell.

// Places that are grass during warm seasons become snowy during cold seasons. 
// Hmm. The opposite isn't true, I'll wager. It's easy to tell if a place is
// normally grassy because the buildings will retain their original style.

// Indoor locations don't have a climate. The main quest dungeons don't have a
// climate, and should just use one that makes sense. They'll probably end up having
// custom weather (like Elden Grove fog) anyway. But really, a climate and the 
// season will determine the tileset.

// Maybe there should be a "WeatherName" enum, with things like "Clear", "Fog", etc..

enum class ClimateName
{
	Cold,
	Desert,
	Grassy,
	Snowy
};

#endif
