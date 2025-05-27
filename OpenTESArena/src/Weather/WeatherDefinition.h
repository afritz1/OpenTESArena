#ifndef WEATHER_DEFINITION_H
#define WEATHER_DEFINITION_H

#include "../Assets/ArenaTypes.h"

#include "components/utilities/Buffer.h"

class Random;

enum class WeatherType
{
	Clear,
	Overcast,
	Rain,
	Snow
};

struct WeatherOvercastDefinition
{
	bool heavyFog;

	void init(bool heavyFog);
};

struct WeatherRainDefinition
{
	bool thunderstorm;

	void init(bool thunderstorm);
};

struct WeatherSnowDefinition
{
	bool overcast; // Affects music.
	bool heavyFog;

	void init(bool overcast, bool heavyFog);
};

struct WeatherDefinition
{
	WeatherType type;

	union
	{
		WeatherOvercastDefinition overcast;
		WeatherRainDefinition rain;
		WeatherSnowDefinition snow;
	};

	WeatherDefinition();

	bool operator==(const WeatherDefinition &other) const;

	void initClear();
	void initOvercast(bool heavyFog);
	void initRain(bool thunderstorm);
	void initSnow(bool overcast, bool heavyFog);
	void initFromClassic(ArenaTypes::WeatherType weatherType, int currentDay, Random &random);
};

#endif
