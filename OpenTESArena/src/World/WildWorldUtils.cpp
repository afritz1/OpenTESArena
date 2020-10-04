#include <cstdio>

#include "ClimateType.h"
#include "WeatherType.h"
#include "WeatherUtils.h"
#include "WildWorldUtils.h"

#include "components/debug/Debug.h"

DOSUtils::FilenameBuffer WildWorldUtils::generateInfName(ClimateType climateType,
	WeatherType weatherType)
{
	const char climateLetter = [climateType]()
	{
		if (climateType == ClimateType::Temperate)
		{
			return 'T';
		}
		else if (climateType == ClimateType::Desert)
		{
			return 'D';
		}
		else if (climateType == ClimateType::Mountain)
		{
			return 'M';
		}
		else
		{
			DebugUnhandledReturnMsg(char, std::to_string(static_cast<int>(climateType)));
		}
	}();

	// Wilderness is "W".
	const char locationLetter = 'W';

	const char weatherLetter = [climateType, weatherType]()
	{
		if (WeatherUtils::isClear(weatherType) || WeatherUtils::isOvercast(weatherType))
		{
			return 'N';
		}
		else if (WeatherUtils::isRain(weatherType))
		{
			return 'R';
		}
		else if (WeatherUtils::isSnow(weatherType))
		{
			// Deserts can't have snow.
			if (climateType != ClimateType::Desert)
			{
				return 'S';
			}
			else
			{
				DebugLogWarning("Deserts do not have snow templates.");
				return 'N';
			}
		}
		else
		{
			// Not sure what this means.
			return 'W';
		}
	}();

	DOSUtils::FilenameBuffer buffer;
	std::snprintf(buffer.data(), buffer.size(), "%C%C%C.INF",
		climateLetter, locationLetter, weatherLetter);

	return buffer;
}
