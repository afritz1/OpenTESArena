#include "ArenaRenderUtils.h"

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
