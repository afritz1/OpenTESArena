#include <cmath>

#include "ArenaSkyUtils.h"
#include "SkyUtils.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/TextureManager.h"
#include "../Math/Constants.h"
#include "../Weather/WeatherDefinition.h"

#include "components/debug/Debug.h"

int SkyUtils::getOctantIndex(bool posX, bool posY, bool posZ)
{
	// Use lowest 3 bits to represent 0-7.
	const char xBit = posX ? 0 : (1 << 0);
	const char yBit = posY ? 0 : (1 << 1);
	const char zBit = posZ ? 0 : (1 << 2);
	return xBit | yBit | zBit;
}

VoxelDouble3 SkyUtils::getSkyObjectDirection(Radians angleX, Radians angleY)
{
	return VoxelDouble3(
		-std::sin(angleX),
		std::sin(angleY),
		-std::cos(angleX)).normalized();
}

void SkyUtils::getSkyObjectDimensions(int imageWidth, int imageHeight, double *outWidth, double *outHeight)
{
	*outWidth = static_cast<double>(imageWidth) / ArenaSkyUtils::IDENTITY_DIM;
	*outHeight = static_cast<double>(imageHeight) / ArenaSkyUtils::IDENTITY_DIM;
}

int SkyUtils::getStarCountFromDensity(int starDensity)
{
	if (starDensity == 0)
	{
		// Classic.
		return 40;
	}
	else if (starDensity == 1)
	{
		// Moderate.
		return 1000;
	}
	else if (starDensity == 2)
	{
		// High.
		return 8000;
	}
	else
	{
		DebugUnhandledReturnMsg(int, std::to_string(starDensity));
	}
}

Buffer<Color> SkyUtils::makeInteriorSkyColors(bool isOutdoorDungeon, TextureManager &textureManager)
{
	// Interior sky color comes from the darkest row of an .LGT light palette.
	const char *lightPaletteName = isOutdoorDungeon ? "FOG.LGT" : "NORMAL.LGT";

	const std::optional<TextureBuilderIdGroup> textureBuilderIDs = textureManager.tryGetTextureBuilderIDs(lightPaletteName);
	if (!textureBuilderIDs.has_value())
	{
		DebugLogWarning("Couldn't get texture builder IDs for \"" + std::string(lightPaletteName) + "\".");
		return Buffer<Color>();
	}

	// Get darkest light palette and a suitable color for 'dark'.
	const TextureBuilderID darkestTextureBuilderID = textureBuilderIDs->getID(textureBuilderIDs->getCount() - 1);
	const TextureBuilder &lightPaletteTextureBuilder = textureManager.getTextureBuilderHandle(darkestTextureBuilderID);
	DebugAssert(lightPaletteTextureBuilder.getType() == TextureBuilder::Type::Paletted);
	const TextureBuilder::PalettedTexture &lightPaletteTexture = lightPaletteTextureBuilder.getPaletted();
	const Buffer2D<uint8_t> &lightPaletteTexels = lightPaletteTexture.texels;
	const uint8_t lightColor = lightPaletteTexels.get(16, 0);

	const std::string &paletteName = ArenaPaletteName::Default;
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteName.c_str());
	if (!paletteID.has_value())
	{
		DebugLogWarning("Couldn't get palette ID for \"" + paletteName + "\".");
		return Buffer<Color>();
	}

	const Palette &palette = textureManager.getPaletteHandle(*paletteID);
	DebugAssertIndex(palette, lightColor);
	const Color &paletteColor = palette[lightColor];

	Buffer<Color> skyColors(1);
	skyColors.set(0, paletteColor);
	return skyColors;
}

Buffer<Color> SkyUtils::makeExteriorSkyColors(const WeatherDefinition &weatherDef, TextureManager &textureManager)
{
	// Get the palette name for the given weather.
	const std::string &paletteName = (weatherDef.getType() == WeatherDefinition::Type::Clear) ?
		ArenaPaletteName::Daytime : ArenaPaletteName::Dreary;

	// The palettes in the data files only cover half of the day, so some added darkness is
	// needed for the other half.
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteName.c_str());
	if (!paletteID.has_value())
	{
		DebugLogWarning("Couldn't get palette ID for \"" + paletteName + "\".");
		return Buffer<Color>();
	}

	const Palette &palette = textureManager.getPaletteHandle(*paletteID);

	// Fill sky palette with darkness. The first color in the palette is the closest to night.
	const Color &darkness = palette[0];
	Buffer<Color> fullPalette(static_cast<int>(palette.size()) * 2);
	fullPalette.fill(darkness);

	// Copy the sky palette over the center of the full palette.
	std::copy(palette.begin(), palette.end(), fullPalette.get() + (fullPalette.getCount() / 4));

	return fullPalette;
}
