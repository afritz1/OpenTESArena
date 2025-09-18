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

enum class FragmentShaderType
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

static constexpr FragmentShaderType OBJECT_FRAGMENT_SHADER_TYPE_MAX = FragmentShaderType::AlphaTestedWithHorizonMirrorSecondPass;
static constexpr int OBJECT_FRAGMENT_SHADER_TYPE_COUNT = static_cast<int>(OBJECT_FRAGMENT_SHADER_TYPE_MAX) + 1;
static constexpr FragmentShaderType UI_FRAGMENT_SHADER_TYPE_MAX = FragmentShaderType::UiTexture;
static constexpr int UI_FRAGMENT_SHADER_TYPE_COUNT = static_cast<int>(UI_FRAGMENT_SHADER_TYPE_MAX) + 1 - OBJECT_FRAGMENT_SHADER_TYPE_COUNT;
static constexpr int TOTAL_FRAGMENT_SHADER_TYPE_COUNT = OBJECT_FRAGMENT_SHADER_TYPE_COUNT + UI_FRAGMENT_SHADER_TYPE_COUNT;

enum class DitheringMode
{
	None,
	Classic,
	Modern
};

static constexpr int DITHER_MODE_COUNT = 3;
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
	constexpr bool isOpaque(FragmentShaderType type)
	{
		return (type == FragmentShaderType::Opaque) ||
			(type == FragmentShaderType::OpaqueWithAlphaTestLayer) ||
			(type == FragmentShaderType::OpaqueScreenSpaceAnimation) ||
			(type == FragmentShaderType::OpaqueScreenSpaceAnimationWithAlphaTestLayer);
	}

	constexpr bool requiresMeshLightPercent(FragmentShaderType type)
	{
		return (type == FragmentShaderType::Opaque) ||
			(type == FragmentShaderType::OpaqueScreenSpaceAnimation) ||
			(type == FragmentShaderType::OpaqueScreenSpaceAnimationWithAlphaTestLayer) ||
			(type == FragmentShaderType::AlphaTested);
	}

	constexpr bool requiresTexCoordAnimPercent(FragmentShaderType type)
	{
		return (type == FragmentShaderType::AlphaTestedWithVariableTexCoordUMin) ||
			(type == FragmentShaderType::AlphaTestedWithVariableTexCoordVMin);
	}
}

#endif
