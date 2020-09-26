#include "ClimateType.h"
#include "WeatherType.h"
#include "WildWorldUtils.h"

#include "components/debug/Debug.h"

std::string WildWorldUtils::generateInfName(ClimateType climateType, WeatherType weatherType)
{
	// @todo: use DOSUtils::FilenameBuffer instead

	const std::string climateLetter = [climateType]()
	{
		if (climateType == ClimateType::Temperate)
		{
			return "T";
		}
		else if (climateType == ClimateType::Desert)
		{
			return "D";
		}
		else
		{
			return "M";
		}
	}();

	// Wilderness is "W".
	const std::string locationLetter = "W";

	const std::string weatherLetter = [climateType, weatherType]()
	{
		if ((weatherType == WeatherType::Clear) ||
			(weatherType == WeatherType::Overcast) ||
			(weatherType == WeatherType::Overcast2))
		{
			return "N";
		}
		else if ((weatherType == WeatherType::Rain) ||
			(weatherType == WeatherType::Rain2))
		{
			return "R";
		}
		else if ((weatherType == WeatherType::Snow) ||
			(weatherType == WeatherType::SnowOvercast) ||
			(weatherType == WeatherType::SnowOvercast2))
		{
			// Deserts can't have snow.
			if (climateType != ClimateType::Desert)
			{
				return "S";
			}
			else
			{
				DebugLogWarning("Deserts do not have snow templates.");
				return "N";
			}
		}
		else
		{
			// Not sure what this letter represents.
			return "W";
		}
	}();

	return climateLetter + locationLetter + weatherLetter + ".INF";
}
