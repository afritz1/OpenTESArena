#include "WeatherDefinition.h"
#include "WeatherUtils.h"

WeatherDefinition WeatherUtils::getFilteredWeather(const WeatherDefinition &weatherDef,
	ArenaTypes::ClimateType climateType)
{
	// Snow in deserts is replaced by rain.
	if ((weatherDef.getType() == WeatherDefinition::Type::Snow) && (climateType == ArenaTypes::ClimateType::Desert))
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
