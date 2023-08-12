#ifndef LEGACY_RENDERER_UTILS_H
#define LEGACY_RENDERER_UTILS_H

#include <cstdint>

#include "ArenaRenderUtils.h"

// Temp namespace for storing old code from the 2.5D ray caster, etc., to be deleted once no longer needed.

namespace LegacyRendererUtils
{
	// Low-level fog matrix sampling function.
	template<int TextureWidth, int TextureHeight>
	uint8_t sampleFogMatrixTexture(const ArenaRenderUtils::FogMatrix &fogMatrix, double u, double v);
}

#endif
