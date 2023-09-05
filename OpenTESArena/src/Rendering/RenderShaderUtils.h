#ifndef RENDER_SHADER_UTILS_H
#define RENDER_SHADER_UTILS_H

enum class VertexShaderType
{
	Voxel,
	SwingingDoor,
	SlidingDoor,
	RaisingDoor,
	SplittingDoor,
	Entity
};

enum class PixelShaderType
{
	Opaque,
	OpaqueWithAlphaTestLayer, // Chasm walls.
	AlphaTested,
	AlphaTestedWithVariableTexCoordUMin, // Sliding doors.
	AlphaTestedWithVariableTexCoordVMin, // Raising doors.
	AlphaTestedWithPaletteIndexLookup, // Citizens.
	AlphaTestedWithLightLevelColor, // Clouds, distant moons.
	AlphaTestedWithLightLevelOpacity, // Ghosts, screen-space fog.
	AlphaTestedWithPreviousBrightnessLimit // Stars.
};

enum class TextureSamplingType
{
	Default,
	ScreenSpaceRepeatY // Chasms.
};

using UniformBufferID = int;

// Unique ID for a light allocated in the renderer's internal format.
using RenderLightID = int;

enum class RenderLightingType
{
	PerMesh, // Mesh is uniformly shaded by a single draw call value.
	PerPixel // Mesh is shaded by lights in the scene.
};

#endif
