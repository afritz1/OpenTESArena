#ifndef RENDER_LIGHT_UTILS_H
#define RENDER_LIGHT_UTILS_H

// Unique ID for a light allocated in the renderer's internal format.
using RenderLightID = int;

enum class RenderLightingType
{
	PerMesh, // Mesh is uniformly shaded by a single draw call value.
	PerPixel // Mesh is shaded by lights in the scene.
};

#endif
