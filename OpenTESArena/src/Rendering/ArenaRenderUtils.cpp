#include <algorithm>
#include <optional>

#include "ArenaRenderUtils.h"
#include "../Assets/TextureBuilder.h"
#include "../Assets/TextureManager.h"
#include "../Game/ArenaClockUtils.h"
#include "../Math/Constants.h"
#include "../Math/Random.h"
#include "../Math/Vector4.h"
#include "../World/MapType.h"

#include "components/debug/Debug.h"
#include "components/utilities/Bytes.h"

double ArenaRenderUtils::getAmbientPercent(const Clock &clock, MapType mapType)
{
	if (mapType == MapType::Interior)
	{
		return 0.0;
	}
	else if ((mapType == MapType::City) || (mapType == MapType::Wilderness))
	{
		const double clockPreciseSeconds = clock.getPreciseTotalSeconds();

		// Time ranges where the ambient light changes. The start times are inclusive, and the end times are exclusive.
		const double startBrighteningTime = ArenaClockUtils::AmbientStartBrightening.getPreciseTotalSeconds();
		const double endBrighteningTime = ArenaClockUtils::AmbientEndBrightening.getPreciseTotalSeconds();
		const double startDimmingTime = ArenaClockUtils::AmbientStartDimming.getPreciseTotalSeconds();
		const double endDimmingTime = ArenaClockUtils::AmbientEndDimming.getPreciseTotalSeconds();

		constexpr double minAmbient = 0.0;
		constexpr double maxAmbient = 1.0;

		double ambient;
		if ((clockPreciseSeconds >= endBrighteningTime) && (clockPreciseSeconds < startDimmingTime))
		{
			// Daytime ambient.
			ambient = maxAmbient;
		}
		else if ((clockPreciseSeconds >= startBrighteningTime) && (clockPreciseSeconds < endBrighteningTime))
		{
			// Interpolate brightening light (in the morning).
			const double timePercent = (clockPreciseSeconds - startBrighteningTime) / (endBrighteningTime - startBrighteningTime);
			ambient = minAmbient + ((maxAmbient - minAmbient) * timePercent);
		}
		else if ((clockPreciseSeconds >= startDimmingTime) && (clockPreciseSeconds < endDimmingTime))
		{
			// Interpolate dimming light (in the evening).
			const double timePercent = (clockPreciseSeconds - startDimmingTime) / (endDimmingTime - startDimmingTime);
			ambient = maxAmbient + ((minAmbient - maxAmbient) * timePercent);
		}
		else
		{
			// Night ambient.
			ambient = minAmbient;
		}

		return std::clamp(ambient, minAmbient, maxAmbient);
	}
	else
	{
		DebugUnhandledReturnMsg(double, std::to_string(static_cast<int>(mapType)));
	}
}

double ArenaRenderUtils::getDistantAmbientPercent(double ambientPercent)
{
	return std::clamp(ambientPercent, 0.15, 1.0);
}

bool ArenaRenderUtils::isLightLevelTexel(uint8_t texel)
{
	return (texel >= ArenaRenderUtils::PALETTE_INDEX_LIGHT_LEVEL_LOWEST) &&
		(texel <= ArenaRenderUtils::PALETTE_INDEX_LIGHT_LEVEL_HIGHEST);
}

bool ArenaRenderUtils::isPuddleTexel(uint8_t texel)
{
	return (texel == ArenaRenderUtils::PALETTE_INDEX_PUDDLE_EVEN_ROW) ||
		(texel == ArenaRenderUtils::PALETTE_INDEX_PUDDLE_ODD_ROW);
}

bool ArenaRenderUtils::tryMakeFogMatrix(int zeroedRow, Random &random, TextureManager &textureManager,
	FogMatrix *outMatrix)
{
	DebugAssert(zeroedRow >= 0);
	DebugAssert(zeroedRow < ArenaRenderUtils::FOG_MATRIX_HEIGHT);

	const std::string filename = "FOG.TXT";
	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(filename.c_str());
	if (!textureBuilderID.has_value())
	{
		DebugLogError("Couldn't get texture builder ID for \"" + filename + "\".");
		return false;
	}

	// The fog texture is 16 bits per pixel but it's expanded to 32-bit so the engine doesn't have to support
	// another texture builder format only used here.
	const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(*textureBuilderID);
	const TextureBuilder::TrueColorTexture &texture = textureBuilder.getTrueColor();
	const Buffer2D<uint32_t> &texels = texture.texels;
	const uint32_t *pixelMaxPtr = std::max_element(texels.begin(), texels.end());
	const uint8_t pixelMax = static_cast<uint8_t>((*pixelMaxPtr) >> 8);

	// Generate random pixel values based on the max.
	const int pixelCount = ArenaRenderUtils::FOG_MATRIX_WIDTH * ArenaRenderUtils::FOG_MATRIX_HEIGHT;
	for (int i = 0; i < pixelCount; i++)
	{
		const uint16_t texel = Bytes::getLE16(reinterpret_cast<const uint8_t*>(texels.begin() + i));
		const uint8_t highByte = (texel >> 8) & 0xFF;
		const uint8_t lowByte = texel & 0xFF;

		const uint8_t paletteIndex = static_cast<uint8_t>(random.next(pixelMax)); // Not using FOG.TXT yet.
		(*outMatrix)[i] = paletteIndex;
	}

	// Zero out one of the rows.
	constexpr int matrixWidth = ArenaRenderUtils::FOG_MATRIX_WIDTH;
	for (int x = 0; x < matrixWidth; x++)
	{
		const int y = zeroedRow;
		const int index = x + (y * matrixWidth);
		(*outMatrix)[index] = 0;
	}

	return true;
}

void ArenaRenderUtils::drawFog(const FogMatrix &fogMatrix, Random &random, uint8_t *outPixels)
{
	constexpr int matrixSurfaceWidth = ArenaRenderUtils::FOG_MATRIX_WIDTH * ArenaRenderUtils::FOG_MATRIX_SCALE;
	constexpr int matrixSurfaceHeight = ArenaRenderUtils::FOG_MATRIX_HEIGHT * ArenaRenderUtils::FOG_MATRIX_SCALE;
	constexpr int textureWidth = ArenaRenderUtils::FOG_MATRIX_WIDTH;
	constexpr int textureHeight = ArenaRenderUtils::FOG_MATRIX_HEIGHT;
	constexpr double textureWidthReal = static_cast<double>(textureWidth);
	constexpr double textureHeightReal = static_cast<double>(textureHeight);

	// Linear texture-filtered sample in fog matrix.
	auto sampleFogMatrixTexture = [&fogMatrix, textureWidth, textureHeight, textureWidthReal,
		textureHeightReal](double u, double v)
	{
		const double texelWidth = 1.0 / textureWidthReal;
		const double texelHeight = 1.0 / textureHeightReal;
		const double halfTexelWidth = texelWidth * 0.50;
		const double halfTexelHeight = texelHeight * 0.50;

		// Neighboring percents that might land in an adjacent texel.
		const double uLow = std::max(u - halfTexelWidth, 0.0);
		const double uHigh = std::min(u + halfTexelWidth, Constants::JustBelowOne);
		const double vLow = std::max(v - halfTexelHeight, 0.0);
		const double vHigh = std::min(v + halfTexelHeight, Constants::JustBelowOne);

		const double uLowWidth = uLow * textureWidthReal;
		const double vLowHeight = vLow * textureHeightReal;
		const double uLowPercent = 1.0 - (uLowWidth - std::floor(uLowWidth));
		const double uHighPercent = 1.0 - uLowPercent;
		const double vLowPercent = 1.0 - (vLowHeight - std::floor(vLowHeight));
		const double vHighPercent = 1.0 - vLowPercent;
		const double tlPercent = uLowPercent * vLowPercent;
		const double trPercent = uHighPercent * vLowPercent;
		const double blPercent = uLowPercent * vHighPercent;
		const double brPercent = uHighPercent * vHighPercent;
		const int textureXL = static_cast<int>(uLow * textureWidthReal);
		const int textureXR = static_cast<int>(uHigh * textureWidthReal);
		const int textureYT = static_cast<int>(vLow * textureHeightReal);
		const int textureYB = static_cast<int>(vHigh * textureHeightReal);
		const int textureIndexTL = textureXL + (textureYT * textureWidth);
		const int textureIndexTR = textureXR + (textureYT * textureWidth);
		const int textureIndexBL = textureXL + (textureYB * textureWidth);
		const int textureIndexBR = textureXR + (textureYB * textureWidth);

		const uint8_t texelTL = fogMatrix[textureIndexTL];
		const uint8_t texelTR = fogMatrix[textureIndexTR];
		const uint8_t texelBL = fogMatrix[textureIndexBL];
		const uint8_t texelBR = fogMatrix[textureIndexBR];

		constexpr int percentMultiplier = 100;
		constexpr double percentMultiplierReal = static_cast<double>(percentMultiplier);
		const uint16_t tlPercentInteger = static_cast<uint16_t>(tlPercent * percentMultiplierReal);
		const uint16_t trPercentInteger = static_cast<uint16_t>(trPercent * percentMultiplierReal);
		const uint16_t blPercentInteger = static_cast<uint16_t>(blPercent * percentMultiplierReal);
		const uint16_t brPercentInteger = static_cast<uint16_t>(brPercent * percentMultiplierReal);

		const uint16_t texelTLScaled = texelTL * tlPercentInteger;
		const uint16_t texelTRScaled = texelTR * trPercentInteger;
		const uint16_t texelBLScaled = texelBL * blPercentInteger;
		const uint16_t texelBRScaled = texelBR * brPercentInteger;

		const uint16_t texelSumScaled = texelTLScaled + texelTRScaled + texelBLScaled + texelBRScaled;
		return static_cast<uint8_t>(texelSumScaled / percentMultiplier);
	};

	for (int y = 0; y < textureHeight; y++)
	{
		const int yOffset = y * ArenaRenderUtils::FOG_MATRIX_SCALE;

		for (int x = 0; x < textureWidth; x++)
		{
			const int xOffset = x * ArenaRenderUtils::FOG_MATRIX_SCALE;

			for (int i = 0; i < ArenaRenderUtils::FOG_MATRIX_SCALE; i++)
			{
				for (int j = 0; j < ArenaRenderUtils::FOG_MATRIX_SCALE; j++)
				{
					int textureX, textureY;
					if (((i + j) & 1) != 0)
					{
						textureX = x;
						textureY = y;
					}
					else
					{
						textureX = random.next(textureWidth);
						textureY = random.next(textureHeight);
					}

					const double uTexture = static_cast<double>(textureX) +
						((static_cast<double>(i) + 0.50) / static_cast<double>(ArenaRenderUtils::FOG_MATRIX_SCALE));
					const double vTexture = static_cast<double>(textureY) +
						((static_cast<double>(j) + 0.50) / static_cast<double>(ArenaRenderUtils::FOG_MATRIX_SCALE));

					const double u = std::clamp(uTexture / textureWidthReal, 0.0, Constants::JustBelowOne);
					const double v = std::clamp(vTexture / textureHeightReal, 0.0, Constants::JustBelowOne);
					const uint8_t paletteIndex = sampleFogMatrixTexture(u, v);

					const int dstIndex = (xOffset + i) + ((yOffset + j) * matrixSurfaceWidth);
					outPixels[dstIndex] = paletteIndex;
				}
			}
		}
	}
}
