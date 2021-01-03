#include <unordered_map>

#include "ClimateType.h"
#include "WeatherType.h"
#include "WeatherUtils.h"
#include "../Media/TextureManager.h"

namespace
{
	// Arbitrary fog distances for each weather; the distance at which fog is maximum.
	// @todo: make this match the original game more closely? They are currently arbitrary.
	const std::unordered_map<WeatherType, double> WeatherFogDistances =
	{
		{ WeatherType::Clear, 100.0 },
		{ WeatherType::Overcast, 30.0 },
		{ WeatherType::Rain, 50.0 },
		{ WeatherType::Snow, 25.0 },
		{ WeatherType::SnowOvercast, 20.0 },
		{ WeatherType::Rain2, 50.0 },
		{ WeatherType::Overcast2, 30.0 },
		{ WeatherType::SnowOvercast2, 20.0 }
	};
}

bool WeatherUtils::isClear(WeatherType weatherType)
{
	return weatherType == WeatherType::Clear;
}

bool WeatherUtils::isOvercast(WeatherType weatherType)
{
	return (weatherType == WeatherType::Overcast) || (weatherType == WeatherType::Overcast2);
}

bool WeatherUtils::isRain(WeatherType weatherType)
{
	return (weatherType == WeatherType::Rain) || (weatherType == WeatherType::Rain2);
}

bool WeatherUtils::isSnow(WeatherType weatherType)
{
	return (weatherType == WeatherType::Snow) || (weatherType == WeatherType::SnowOvercast) ||
		(weatherType == WeatherType::SnowOvercast2);
}

WeatherType WeatherUtils::getFilteredWeatherType(WeatherType weatherType, ClimateType climateType)
{
	// Snow in deserts is replaced by rain.
	const bool isSnow = WeatherUtils::isSnow(weatherType);
	return ((climateType == ClimateType::Desert) && isSnow) ? WeatherType::Rain : weatherType;
}

double WeatherUtils::getFogDistanceFromWeather(WeatherType weatherType)
{
	return WeatherFogDistances.at(weatherType);
}

Buffer<uint32_t> WeatherUtils::makeExteriorSkyPalette(WeatherType weatherType,
	TextureManager &textureManager)
{
	// Get the palette name for the given weather.
	const char *paletteName = (weatherType == WeatherType::Clear) ? "DAYTIME.COL" : "DREARY.COL";

	// The palettes in the data files only cover half of the day, so some added
	// darkness is needed for the other half.
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteName);
	if (!paletteID.has_value())
	{
		DebugCrash("Couldn't get palette ID for \"" + std::string(paletteName) + "\".");
	}

	const Palette &palette = textureManager.getPaletteHandle(*paletteID);

	// Fill sky palette with darkness (the first color in the palette is the closest to night).
	const uint32_t darkness = palette[0].toARGB();
	Buffer<uint32_t> fullPalette(static_cast<int>(palette.size()) * 2);
	fullPalette.fill(darkness);

	// Copy the sky palette over the center of the full palette.
	std::transform(palette.begin(), palette.end(), fullPalette.get() + (fullPalette.getCount() / 4),
		[](const Color &color) { return color.toARGB(); });

	return fullPalette;
}
