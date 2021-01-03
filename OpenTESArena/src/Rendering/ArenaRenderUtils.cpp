#include "ArenaRenderUtils.h"

bool ArenaRenderUtils::IsGhostTexel(uint8_t texel)
{
	return (texel >= ArenaRenderUtils::PALETTE_INDEX_LIGHT_LEVEL_LOWEST) &&
		(texel <= ArenaRenderUtils::PALETTE_INDEX_LIGHT_LEVEL_HIGHEST);
}

bool ArenaRenderUtils::IsPuddleTexel(uint8_t texel)
{
	return (texel == ArenaRenderUtils::PALETTE_INDEX_PUDDLE_EVEN_ROW) ||
		(texel == ArenaRenderUtils::PALETTE_INDEX_PUDDLE_ODD_ROW);
}

bool ArenaRenderUtils::IsCloudTexel(uint8_t texel)
{
	return (texel >= ArenaRenderUtils::PALETTE_INDEX_SKY_LEVEL_LOWEST) &&
		(texel <= ArenaRenderUtils::PALETTE_INDEX_SKY_LEVEL_HIGHEST);
}
