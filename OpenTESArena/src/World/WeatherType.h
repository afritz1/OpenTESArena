#ifndef WEATHER_TYPE_H
#define WEATHER_TYPE_H

// A unique identifier for each kind of weather. These can have an effect on the sky
// palette, fog distance, music, etc..

// If in terrain type 3, snow and snow overcast are replaced by rain.

enum class WeatherType
{
	Clear,
	Overcast, // With fog.
	Rain,
	Snow,
	SnowOvercast, // With fog.
	Rain2,
	Overcast2,
	SnowOvercast2 // If rnd() < 16000 then with fog.
};

#endif
