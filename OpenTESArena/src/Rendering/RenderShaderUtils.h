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
	OpaqueWithAlphaTestLayer, // Dry chasm walls.
	OpaqueScreenSpaceAnimation, // Water/lava chasm floors.
	OpaqueScreenSpaceAnimationWithAlphaTestLayer, // Water/lava chasm walls.
	AlphaTested,
	AlphaTestedWithVariableTexCoordUMin, // Sliding doors.
	AlphaTestedWithVariableTexCoordVMin, // Raising doors.
	AlphaTestedWithPaletteIndexLookup, // Citizens.
	AlphaTestedWithLightLevelColor, // Clouds, distant moons.
	AlphaTestedWithLightLevelOpacity, // Ghosts, screen-space fog.
	AlphaTestedWithPreviousBrightnessLimit, // Stars.
	AlphaTestedWithHorizonMirror // Puddles.
};

static constexpr PixelShaderType PIXEL_SHADER_TYPE_MAX = PixelShaderType::AlphaTestedWithHorizonMirror;
static constexpr int PIXEL_SHADER_TYPE_COUNT = static_cast<int>(PIXEL_SHADER_TYPE_MAX) + 1;

enum class DitheringMode
{
	None,
	Classic,
	Modern
};

static constexpr int DITHERING_MODERN_MASK_COUNT = 4;

using UniformBufferID = int;

#endif
