#include "WeatherDefinition.h"
#include "WeatherUtils.h"

WeatherDefinition WeatherUtils::getFilteredWeather(const WeatherDefinition &weatherDef,
	ArenaClimateType climateType)
{
	// Snow in deserts is replaced by rain.
	if ((weatherDef.type == WeatherType::Snow) && (climateType == ArenaClimateType::Desert))
	{
		constexpr bool thunderstorm = false;
		WeatherDefinition filteredWeatherDef;
		filteredWeatherDef.initRain(thunderstorm);
		return filteredWeatherDef;
	}
	else
	{
		return weatherDef;
	}
}
