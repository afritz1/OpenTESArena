#ifndef WEATHER_DEFINITION_H
#define WEATHER_DEFINITION_H

#include "../Assets/ArenaTypes.h"

#include "components/utilities/Buffer.h"

class Color;
class Random;

class WeatherDefinition
{
public:
	enum class Type
	{
		Clear,
		Overcast,
		Rain,
		Snow
	};

	struct OvercastDefinition
	{
		bool heavyFog;

		void init(bool heavyFog);
	};

	struct RainDefinition
	{
		bool thunderstorm;

		void init(bool thunderstorm);
	};

	struct SnowDefinition
	{
		bool overcast; // Affects music.
		bool heavyFog;

		void init(bool overcast, bool heavyFog);
	};
private:
	Type type;

	union
	{
		OvercastDefinition overcast;
		RainDefinition rain;
		SnowDefinition snow;
	};
public:
	WeatherDefinition();

	bool operator==(const WeatherDefinition &other) const;

	void initClear();
	void initOvercast(bool heavyFog);
	void initRain(bool thunderstorm);
	void initSnow(bool overcast, bool heavyFog);

	Type getType() const;
	double getFogDistance() const;
	const OvercastDefinition &getOvercast() const;
	const RainDefinition &getRain() const;
	const SnowDefinition &getSnow() const;
};

#endif
