#ifndef RENDER_SHADER_UTILS_H
#define RENDER_SHADER_UTILS_H

enum class VertexShaderType
{
	Basic,
	Entity,
	UI
};

static constexpr VertexShaderType VERTEX_SHADER_TYPE_MAX = VertexShaderType::Entity;
static constexpr int VERTEX_SHADER_TYPE_COUNT = static_cast<int>(VERTEX_SHADER_TYPE_MAX) + 1;

enum class PixelShaderType
{
	// Object textures
	Opaque, // Most walls/floors/ceilings.
	OpaqueWithAlphaTestLayer, // Dry chasm walls.
	OpaqueScreenSpaceAnimation, // Water/lava chasm floors.
	OpaqueScreenSpaceAnimationWithAlphaTestLayer, // Water/lava chasm walls.
	AlphaTested, // Enemies.
	AlphaTestedWithVariableTexCoordUMin, // Sliding doors.
	AlphaTestedWithVariableTexCoordVMin, // Raising doors.
	AlphaTestedWithPaletteIndexLookup, // Citizens.
	AlphaTestedWithLightLevelOpacity, // Ghosts, screen-space fog.
	AlphaTestedWithPreviousBrightnessLimit, // Stars.
	AlphaTestedWithHorizonMirrorFirstPass, // Puddles without reflection.
	AlphaTestedWithHorizonMirrorSecondPass, // Puddle reflections.

	// UI textures
	UiTexture
};

static constexpr PixelShaderType OBJECT_PIXEL_SHADER_TYPE_MAX = PixelShaderType::AlphaTestedWithHorizonMirrorSecondPass;
static constexpr int OBJECT_PIXEL_SHADER_TYPE_COUNT = static_cast<int>(OBJECT_PIXEL_SHADER_TYPE_MAX) + 1;
static constexpr PixelShaderType UI_PIXEL_SHADER_TYPE_MAX = PixelShaderType::UiTexture;
static constexpr int UI_PIXEL_SHADER_TYPE_COUNT = static_cast<int>(UI_PIXEL_SHADER_TYPE_MAX) + 1 - OBJECT_PIXEL_SHADER_TYPE_COUNT;
static constexpr int TOTAL_PIXEL_SHADER_TYPE_COUNT = OBJECT_PIXEL_SHADER_TYPE_COUNT + UI_PIXEL_SHADER_TYPE_COUNT;

enum class DitheringMode
{
	None,
	Classic,
	Modern
};

static constexpr int DITHERING_MODERN_MASK_COUNT = 4;

using UniformBufferID = int;

// Per-draw-call type for framebuffer dependencies like if the previous framebuffer should be provided as an input texture.
enum class RenderMultipassType
{
	None,
	Stars,
	Ghosts,
	Puddles
};

namespace RenderShaderUtils
{
	constexpr bool isOpaque(PixelShaderType type)
	{
		return (type == PixelShaderType::Opaque) ||
			(type == PixelShaderType::OpaqueWithAlphaTestLayer) ||
			(type == PixelShaderType::OpaqueScreenSpaceAnimation) ||
			(type == PixelShaderType::OpaqueScreenSpaceAnimationWithAlphaTestLayer);
	}

	constexpr bool requiresMeshLightPercent(PixelShaderType type)
	{
		return (type == PixelShaderType::Opaque) ||
			(type == PixelShaderType::AlphaTested);
	}

	constexpr bool requiresPixelShaderParam(PixelShaderType type)
	{
		return (type == PixelShaderType::AlphaTestedWithVariableTexCoordUMin) ||
			(type == PixelShaderType::AlphaTestedWithVariableTexCoordVMin);
	}
}

#endif
