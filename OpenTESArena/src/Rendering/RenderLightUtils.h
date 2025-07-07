#ifndef RENDER_LIGHT_UTILS_H
#define RENDER_LIGHT_UTILS_H

// Unique ID for a light allocated in the renderer's internal format.
using RenderLightID = int;

enum class RenderLightingType
{
	PerMesh, // Mesh is uniformly shaded by a single draw call value.
	PerPixel // Mesh is shaded by lights in the scene.
};

static constexpr RenderLightingType RENDER_LIGHTING_TYPE_MAX = RenderLightingType::PerPixel;
static constexpr int RENDER_LIGHTING_TYPE_COUNT = static_cast<int>(RENDER_LIGHTING_TYPE_MAX) + 1;

#endif
