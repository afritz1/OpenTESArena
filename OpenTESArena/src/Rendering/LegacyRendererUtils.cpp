#include <algorithm>
#include <utility>

#include "LegacyRendererUtils.h"
#include "RendererUtils.h"
#include "../Math/Constants.h"
#include "../Math/MathUtils.h"

template<int TextureWidth, int TextureHeight>
uint8_t LegacyRendererUtils::sampleFogMatrixTexture(const ArenaRenderUtils::FogMatrix &fogMatrix, double u, double v)
{
	static_assert((TextureWidth * TextureHeight) == std::tuple_size_v<std::remove_reference_t<decltype(fogMatrix)>>);
	constexpr double textureWidthReal = static_cast<double>(TextureWidth);
	constexpr double textureHeightReal = static_cast<double>(TextureHeight);
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
	const int textureXL = std::clamp(static_cast<int>(uLow * textureWidthReal), 0, TextureWidth - 1);
	const int textureXR = std::clamp(static_cast<int>(uHigh * textureWidthReal), 0, TextureWidth - 1);
	const int textureYT = std::clamp(static_cast<int>(vLow * textureHeightReal), 0, TextureHeight - 1);
	const int textureYB = std::clamp(static_cast<int>(vHigh * textureHeightReal), 0, TextureHeight - 1);
	const int textureIndexTL = textureXL + (textureYT * TextureWidth);
	const int textureIndexTR = textureXR + (textureYT * TextureWidth);
	const int textureIndexBL = textureXL + (textureYB * TextureWidth);
	const int textureIndexBR = textureXR + (textureYB * TextureWidth);

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
}
