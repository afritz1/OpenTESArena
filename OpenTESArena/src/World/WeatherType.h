#ifndef WEATHER_TYPE_H
#define WEATHER_TYPE_H

// A unique identifier for each kind of weather. These can have an effect on the sky
// palette, fog distance, music, etc..
enum class WeatherType
{
	Clear,
	Overcast,
	Rainy,
	Snowy
};

#endif
