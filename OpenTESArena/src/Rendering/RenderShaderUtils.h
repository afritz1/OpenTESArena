#ifndef RENDER_SHADER_UTILS_H
#define RENDER_SHADER_UTILS_H

enum class VertexShaderType
{
	Basic,
	RaisingDoor,
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
	AlphaTestedWithPreviousBrightnessLimit, // Stars.
	AlphaTestedWithHorizonMirror // Puddles.
};

enum class TextureSamplingType
{
	Default,
	ScreenSpaceRepeatY // Chasms.
};

using UniformBufferID = int;

#endif
