#include <algorithm>
#include <optional>

#include "ArenaRenderUtils.h"
#include "../Math/Random.h"
#include "../Media/TextureBuilder.h"
#include "../Media/TextureManager.h"

#include "components/debug/Debug.h"

bool ArenaRenderUtils::isGhostTexel(uint8_t texel)
{
	return (texel >= ArenaRenderUtils::PALETTE_INDEX_LIGHT_LEVEL_LOWEST) &&
		(texel <= ArenaRenderUtils::PALETTE_INDEX_LIGHT_LEVEL_HIGHEST);
}

bool ArenaRenderUtils::isPuddleTexel(uint8_t texel)
{
	return (texel == ArenaRenderUtils::PALETTE_INDEX_PUDDLE_EVEN_ROW) ||
		(texel == ArenaRenderUtils::PALETTE_INDEX_PUDDLE_ODD_ROW);
}

bool ArenaRenderUtils::isCloudTexel(uint8_t texel)
{
	return (texel >= ArenaRenderUtils::PALETTE_INDEX_SKY_LEVEL_LOWEST) &&
		(texel <= ArenaRenderUtils::PALETTE_INDEX_SKY_LEVEL_HIGHEST);
}

bool ArenaRenderUtils::tryMakeFogMatrix(Random &random, TextureManager &textureManager, FogMatrix *outMatrix)
{
	const std::string filename = "FOG.TXT";
	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(filename.c_str());
	if (!textureBuilderID.has_value())
	{
		DebugLogError("Couldn't get texture builder ID for \"" + filename + "\".");
		return false;
	}

	// It's been converted from 16-bit to 32-bit. Find the max then convert to 8-bit because of the right shift.
	const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(*textureBuilderID);
	const TextureBuilder::TrueColorTexture &texture = textureBuilder.getTrueColor();
	const Buffer2D<uint32_t> &texels = texture.texels;
	const uint32_t *pixelMaxPtr = std::max_element(texels.get(), texels.end());
	const uint8_t pixelMax = static_cast<uint8_t>((*pixelMaxPtr) >> 8);

	// Generate random pixel values based on the max.
	std::generate(outMatrix->begin(), outMatrix->end(),
		[&random, pixelMax]()
	{
		return random.next(pixelMax);
	});

	// Zero out one of the rows.
	constexpr int matrixWidth = ArenaRenderUtils::FOG_MATRIX_WIDTH;
	for (int x = 0; x < matrixWidth; x++)
	{
		constexpr int y = ArenaRenderUtils::FOG_MATRIX_ZEROED_ROW;
		const int index = x + (y * matrixWidth);
		(*outMatrix)[index] = 0;
	}

	return true;
}
