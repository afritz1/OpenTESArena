#ifndef RENDER_LIGHT_UTILS_H
#define RENDER_LIGHT_UTILS_H

#include "components/utilities/BufferView.h"

// Unique ID for a light allocated in the renderer's internal format.
using RenderLightID = int;

enum class RenderLightingType
{
	PerMesh, // Mesh is uniformly shaded by a single draw call value.
	PerPixel // Mesh is shaded by lights in the scene.
};

// Lights affecting a specific portion of the scene, like a voxel or entity.
struct RenderLightIdList
{
	static constexpr int MAX_LIGHTS = 8;

	RenderLightID lightIDs[MAX_LIGHTS];
	int lightCount;

	RenderLightIdList();

	BufferView<const RenderLightID> getLightIDs() const;
	void tryAddLight(RenderLightID id);
	void removeLightAt(int index);
	void removeLight(RenderLightID id);
	void clear();
};

#endif
