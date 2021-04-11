#include "ClimateType.h"
#include "WeatherDefinition.h"
#include "WeatherUtils.h"

WeatherDefinition WeatherUtils::getFilteredWeather(const WeatherDefinition &weatherDef, ClimateType climateType)
{
	// Snow in deserts is replaced by rain.
	if ((weatherDef.getType() == WeatherDefinition::Type::Snow) && (climateType == ClimateType::Desert))
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

ArenaTypes::WeatherType WeatherUtils::getLegacyWeather(const WeatherDefinition &weatherDef)
{
	const WeatherDefinition::Type weatherDefType = weatherDef.getType();
	if (weatherDefType == WeatherDefinition::Type::Clear)
	{
		return ArenaTypes::WeatherType::Clear;
	}
	else if (weatherDefType == WeatherDefinition::Type::Overcast)
	{
		const WeatherDefinition::OvercastDefinition &overcastDef = weatherDef.getOvercast();
		return overcastDef.heavyFog ? ArenaTypes::WeatherType::Overcast2 : ArenaTypes::WeatherType::Overcast;
	}
	else if (weatherDefType == WeatherDefinition::Type::Rain)
	{
		const WeatherDefinition::RainDefinition &rainDef = weatherDef.getRain();

		// Not exactly equivalent but good enough for now.
		return rainDef.thunderstorm ? ArenaTypes::WeatherType::Rain2 : ArenaTypes::WeatherType::Rain;
	}
	else if (weatherDefType == WeatherDefinition::Type::Snow)
	{
		const WeatherDefinition::SnowDefinition &snowDef = weatherDef.getSnow();

		// Not exactly equivalent but good enough for now.
		if (snowDef.overcast)
		{
			return snowDef.heavyFog ? ArenaTypes::WeatherType::SnowOvercast2 : ArenaTypes::WeatherType::SnowOvercast;
		}
		else
		{
			return ArenaTypes::WeatherType::Snow;
		}
	}
	else
	{
		DebugUnhandledReturnMsg(ArenaTypes::WeatherType, std::to_string(static_cast<int>(weatherDefType)));
	}
}
