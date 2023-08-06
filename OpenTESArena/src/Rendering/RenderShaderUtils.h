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
	OpaqueWithFade,
	AlphaTested,
	AlphaTestedWithVariableTexCoordUMin, // Sliding doors.
	AlphaTestedWithVariableTexCoordVMin, // Raising doors.
	AlphaTestedWithPaletteIndexLookup, // Citizens.
	AlphaTestedWithLightLevelTransparency // Ghosts, clouds, distant moons.
};

enum class TextureSamplingType
{
	Default,
	ScreenSpaceRepeatY // Chasms.
};

// Unique ID for a light allocated in the renderer's internal format.
using RenderLightID = int;

#endif
