#include <cstdio>
#include <unordered_map>

#include "ArenaWeatherUtils.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ExeData.h"
#include "../Assets/TextureAsset.h"
#include "../Assets/TextureFileMetadata.h"
#include "../Assets/TextureManager.h"
#include "../Math/Random.h"

#include "components/dos/DOSUtils.h"

namespace
{
	// Arbitrary fog distances for each weather; the distance at which fog is maximum.
	// @todo: make this match the original game more closely? They are currently arbitrary.
	const std::unordered_map<ArenaTypes::WeatherType, double> WeatherFogDistances =
	{
		{ ArenaTypes::WeatherType::Clear, 100.0 },
		{ ArenaTypes::WeatherType::Overcast, 30.0 },
		{ ArenaTypes::WeatherType::Rain, 50.0 },
		{ ArenaTypes::WeatherType::Snow, 25.0 },
		{ ArenaTypes::WeatherType::SnowOvercast, 20.0 },
		{ ArenaTypes::WeatherType::Rain2, 50.0 },
		{ ArenaTypes::WeatherType::Overcast2, 30.0 },
		{ ArenaTypes::WeatherType::SnowOvercast2, 20.0 }
	};
}

bool ArenaWeatherUtils::isClear(ArenaTypes::WeatherType weatherType)
{
	return weatherType == ArenaTypes::WeatherType::Clear;
}

bool ArenaWeatherUtils::isOvercast(ArenaTypes::WeatherType weatherType)
{
	return (weatherType == ArenaTypes::WeatherType::Overcast) || (weatherType == ArenaTypes::WeatherType::Overcast2);
}

bool ArenaWeatherUtils::isRain(ArenaTypes::WeatherType weatherType)
{
	return (weatherType == ArenaTypes::WeatherType::Rain) || (weatherType == ArenaTypes::WeatherType::Rain2);
}

bool ArenaWeatherUtils::isSnow(ArenaTypes::WeatherType weatherType)
{
	return (weatherType == ArenaTypes::WeatherType::Snow) || (weatherType == ArenaTypes::WeatherType::SnowOvercast) ||
		(weatherType == ArenaTypes::WeatherType::SnowOvercast2);
}

bool ArenaWeatherUtils::fogIsHeavy(int currentDay)
{
	return (currentDay & 8) != 0;
}

bool ArenaWeatherUtils::rainIsThunderstorm(Random &random)
{
	return random.next(0x10000) < 24000;
}

bool ArenaWeatherUtils::shouldSnowflakeChangeDirection(Random &random)
{
	return random.next(0x10000) < 15000;
}

ArenaTypes::WeatherType ArenaWeatherUtils::getFilteredWeatherType(ArenaTypes::WeatherType weatherType,
	ArenaTypes::ClimateType climateType)
{
	// Snow in deserts is replaced by rain.
	const bool isSnow = ArenaWeatherUtils::isSnow(weatherType);
	return ((climateType == ArenaTypes::ClimateType::Desert) && isSnow) ? ArenaTypes::WeatherType::Rain : weatherType;
}

double ArenaWeatherUtils::getFogDistanceFromWeather(ArenaTypes::WeatherType weatherType)
{
	return WeatherFogDistances.at(weatherType);
}

Buffer<Color> ArenaWeatherUtils::makeSkyColors(ArenaTypes::WeatherType weatherType, TextureManager &textureManager)
{
	// Get the palette name for the given weather.
	const std::string &paletteName = (weatherType == ArenaTypes::WeatherType::Clear) ?
		ArenaPaletteName::Daytime : ArenaPaletteName::Dreary;

	// The palettes in the data files only cover half of the day, so some added
	// darkness is needed for the other half.
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteName.c_str());
	if (!paletteID.has_value())
	{
		DebugCrash("Couldn't get palette ID for \"" + paletteName + "\".");
	}

	const Palette &palette = textureManager.getPaletteHandle(*paletteID);

	// Fill sky palette with darkness (the first color in the palette is the closest to night).
	const Color &darkness = palette[0];
	Buffer<Color> fullPalette(static_cast<int>(palette.size()) * 2);
	fullPalette.fill(darkness);

	// Copy the sky palette over the center of the full palette.
	std::copy(palette.begin(), palette.end(), fullPalette.begin() + (fullPalette.getCount() / 4));
	return fullPalette;
}

Buffer<uint8_t> ArenaWeatherUtils::makeThunderstormColors(const ExeData &exeData)
{
	const auto &srcColors = exeData.weather.thunderstormFlashColors;
	Buffer<uint8_t> colors(static_cast<int>(std::size(srcColors)));
	std::copy(std::begin(srcColors), std::end(srcColors), colors.begin());
	return colors;
}

Buffer<Buffer<TextureAsset>> ArenaWeatherUtils::makeLightningBoltTextureAssets(TextureManager &textureManager)
{
	constexpr int fileCount = 6;
	Buffer<Buffer<TextureAsset>> textureAssetBuffers(fileCount);

	for (int i = 0; i < fileCount; i++)
	{
		DOSUtils::FilenameBuffer filename;
		std::snprintf(filename.data(), filename.size(), "LGLIT0%d.CFA", i + 1);

		Buffer<TextureAsset> textureAssets = TextureUtils::makeTextureAssets(std::string(filename.data()), textureManager);
		textureAssetBuffers.set(i, std::move(textureAssets));
	}

	return textureAssetBuffers;
}
