#ifndef WEATHER_TYPE_H
#define WEATHER_TYPE_H

// A unique identifier for each kind of weather. These can have an effect on the sky
// palette, fog distance, music, etc..

// If in a desert, snow is replaced by rain.

enum class WeatherType
{
	Clear,
	Overcast, // With fog.
	Rain, // If rnd() < 24000 then thunderstorm.
	Snow,
	SnowOvercast, // With fog.
	Rain2, // If rnd() < 24000 then thunderstorm.
	Overcast2,
	SnowOvercast2 // If rnd() < 16000 then with fog.
};

#endif
