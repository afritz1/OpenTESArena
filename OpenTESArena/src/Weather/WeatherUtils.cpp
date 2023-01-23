#include "ArenaWeatherUtils.h"
#include "WeatherDefinition.h"
#include "WeatherUtils.h"

Buffer<WeatherDefinition> WeatherUtils::makeInteriorDefs()
{
	// Interiors only have clear weather.
	Buffer<WeatherDefinition> weatherDefs;
	weatherDefs.init(1);
	weatherDefs.get(0).initClear();
	return weatherDefs;
}

Buffer<WeatherDefinition> WeatherUtils::makeExteriorDefs(ArenaTypes::ClimateType climateType, int currentDay, Random &random)
{
	Buffer<WeatherDefinition> weatherDefs;
	constexpr int clearWeatherDefCount = 1;
	constexpr int overcastWeatherDefCount = 1;
	constexpr int rainWeatherDefCount = 1;

	const bool isDesert = climateType == ArenaTypes::ClimateType::Desert;
	const int snowWeatherDefCount = isDesert ? 0 : 2;

	const int totalWeatherDefCount = clearWeatherDefCount + overcastWeatherDefCount + rainWeatherDefCount + snowWeatherDefCount;
	weatherDefs.init(totalWeatherDefCount);

	WeatherDefinition &clearWeatherDef = weatherDefs.get(0);
	clearWeatherDef.initClear();

	WeatherDefinition &overcastWeatherDef = weatherDefs.get(1);
	const bool isHeavyFog = ArenaWeatherUtils::fogIsHeavy(currentDay);
	overcastWeatherDef.initOvercast(isHeavyFog);

	WeatherDefinition &rainWeatherDef = weatherDefs.get(2);
	const bool isThunderstorm = ArenaWeatherUtils::rainIsThunderstorm(random);
	rainWeatherDef.initRain(isThunderstorm);

	if (!isDesert)
	{
		WeatherDefinition &snowWeatherDef = weatherDefs.get(3);
		snowWeatherDef.initSnow(false, isHeavyFog);

		WeatherDefinition &snowOvercastWeatherDef = weatherDefs.get(4);
		snowOvercastWeatherDef.initSnow(true, isHeavyFog);
	}

	return weatherDefs;
}
