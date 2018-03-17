#ifndef WEATHER_TYPE_H
#define WEATHER_TYPE_H

// A unique identifier for each kind of weather. These can have an effect on the sky
// palette, fog distance, music, etc..
enum class WeatherType
{
	Clear,
	Overcast,
	Rain,
	Snow,
	Unknown0, // Snow overcast?
	Unknown1, // To do.
	Unknown2, // To do (same as overcast?).
	Unknown3 // To do.
};

#endif
